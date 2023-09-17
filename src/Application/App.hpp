#pragma once
#define ARDUINOJSON_USE_LONG_LONG 1
#include <vector> //must be defined before KPServer is included, else error
#include <KPFoundation.hpp>
#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPServer.hpp>
#include <Action.hpp>
#include <Application/Status.hpp>
#include <Application/Constants.hpp>

#include <Application/ScheduleReturnCode.hpp>

#include <Components/Power.hpp>

#include <Valve/Valve.hpp>
#include <Valve/ValveManager.hpp>

#include <Utilities/JsonEncodableDecodable.hpp>

#include <StateControllers/TaskStateController.hpp>
#include <Task/Task.hpp>
#include <Task/TaskManager.hpp>

#include <Application/API.hpp>

class App : public KPController, public TaskObserver {
private:
  void setupServerRouting();

  const char * TaskObserverName() const override {
    return "Application-Task Observer";
  }

  void taskDidUpdate(const Task & task) override {}
  void taskDidDelete(int id) override {
      if (currentTaskId == id) {
          currentTaskId = 0;
      }
  }

  void taskDidComplete() override {}
public:
  KPFileLoader fileLoader{"file-loader", 10}; //SDCS is 10 for Atmel M0
  KPServer server{"web-server", "subsampler", "ilab_sampler"};

  Power power{"power"};
  Config config{ProgramSettings::CONFIG_FILE_PATH};
  Status status;

  TaskStateController taskStateController;


  ValveManager vm;
  TaskManager tm;

  int currentTaskId = 0;


  template <typename T, typename... Args>
  auto dispatchAPI(Args &&... args) {
      return T{}(*this, std::forward<Args>(args)...);
  }

  
  void setup() override {
    Serial.begin(115200);
    while(!Serial) {};

    addComponent(power);
    randomSeed(now());

    
    addComponent(server);
    server.begin();
    setupServerRouting();

    addComponent(ActionScheduler::sharedInstance());
    addComponent(fileLoader);

    //
    // ─── LOADING CONFIG FILE ─────────────────────────────────────────
    //
    // Load configuration from file to initialize config and status objects
    JsonFileLoader loader;
    loader.load(config.configFilepath, config);
    status.init(config);

    // RTC Interrupt callback
    power.onInterrupt([this]() {
        println(GREEN("RTC Interrupted!"));
        println(scheduleNextActiveTask().description());
        interrupts();
    });

    // Regular log header
    if (!SD.exists(config.logFile)) {
        File file = SD.open(config.logFile, FILE_WRITE);
        KPStringBuilder<404> header{"UTC, Formatted Time, Task Name, Valve Number, Current "
                                    "State, Config Sample Time, Config Sample "
                                    "Pressure, Config Sample Volume, Temperature Recorded,"
                                    "Max Pressure Recorded, Volume Recorded, Flow Rate\n"};
        file.println(header);
        file.close();
    }

    // Detail log header
    if (!SD.exists("detail.csv")) {
        File file = SD.open("detail.csv", FILE_WRITE);
        KPStringBuilder<404> header{"UTC, Formatted Time, Task Name, Valve Number, Current "
                                    "State, Config Sample Time, Config Sample "
                                    "Pressure, Config Sample Volume, Temperature Recorded,"
                                    "Pressure Recorded, Volume Recorded, Flow Rate\n"};
        file.println(header);
        file.close();
    }
  }

  
    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief Get the earliest upcoming task and schedule it
     *
     *  @return true if task is successfully scheduled.
     *  @return false if task is either missed schedule or no active task available.
     *  ──────────────────────────────────────────────────────────────────────────── */
    ScheduleReturnCode scheduleNextActiveTask(bool shouldStopCurrentTask = false) {
        status.preventShutdown = false;
        for (auto id : tm.getActiveSortedTaskIds()) {
            Task & task     = tm.tasks[id];
            time_t time_now = now();

            if (currentTaskId == id) {
                // NOTE: Check logic here. Maybe not be correct yet
                if (shouldStopCurrentTask) {
                    cancel("delayTaskExecution");
                    // if (status.currentStateName != HyperFlush::STOP) {
                    // 	newStateController.stop();
                    // }

                    continue;
                } else {
                    status.preventShutdown = true;
                    return ScheduleReturnCode::operating;
                }
            }

            if (time_now >= task.schedule) {
                // Missed schedule
                println(RED("Missed schedule"));
                invalidateTaskAndFreeUpValves(task);
                continue;
            }

            if (time_now >= task.schedule - 10) {                
                // Wake up between 10 secs of the actual schedule time
                // Prepare an action to execute at exact time
                const auto timeUntil = task.schedule - time_now;
                TimedAction delayTaskExecution;
                delayTaskExecution.name     = "delayTaskExecution";
                delayTaskExecution.interval = secsToMillis(timeUntil);
                delayTaskExecution.callback = [this]() { taskStateController.begin(); };
                run(delayTaskExecution);  // async, will be execute later

                taskStateController.configure(task);

                currentTaskId          = id;
                status.preventShutdown = true;
                vm.setValveStatus(task.valves[task.valveOffsetStart], ValveStatus::operating);

                println("\033[32;1mExecuting task in ", timeUntil, " seconds\033[0m");
                return ScheduleReturnCode::operating;
            } else {
                // Wake up before not due to alarm, reschedule anyway
                power.scheduleNextAlarm(task.schedule - 8);  // 3 < x < 10
                return ScheduleReturnCode::scheduled;
            }
        }

        currentTaskId = 0;
        return ScheduleReturnCode::unavailable;
  }

      void validateTaskForSaving(const Task & task, JsonDocument & response) {
        if (task.status == 1) {
            response["error"] = "Task is current active";
            return;
        }

        if (!tm.findTask(task.id)) {
            response["error"] = "Task not found: invalid task id";
            return;
        }
    }

        void validateTaskForScheduling(int id, JsonDocument & response) {
        if (!tm.findTask(id)) {
            response["error"] = "Task not found";
            return;
        }

        Task & task = tm.tasks[id];
        if (task.getNumberOfValves() == 0) {
            response["error"] = "Cannot schedule a task without an assigned valve";
            return;
        }

        if (task.schedule <= now() + 3) {
            response["error"] = "Must be in the future";
            return;
        }

        for (auto v : task.valves) {
            switch (vm.valves[v].status) {
            case ValveStatus::unavailable: {
                KPStringBuilder<100> error("Valve ", v, " is not available");
                response["error"] = (char *) error;
                return;
            }
            case ValveStatus::sampled: {
                KPStringBuilder<100> error("Valve ", v, " has already been sampled");
                response["error"] = (char *) error;
                return;
            }
            case ValveStatus::operating: {
                KPStringBuilder<100> error("Valve ", v, " is operating");
                response["error"] = (char *) error;
                return;
            }
            }
        }
    }

  void invalidateTaskAndFreeUpValves(Task & task) {
        for (auto i = task.getValveOffsetStart(); i < task.getNumberOfValves(); i++) {
            vm.setValveFreeIfNotYetSampled(task.valves[i]);
        }

        task.valves.clear();
        tm.markTaskAsCompleted(task.id);
    }

  void update() override {
    KPController::update();
  };
};