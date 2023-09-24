#include <StateControllers/TaskStateController.hpp>
#include <Application/App.hpp>

void Main::Idle::enter(KPStateMachine & sm) {
    auto & app = *static_cast<App *>(sm.controller);
    println(app.scheduleNextActiveTask().description());
};

void Main::Stop::enter(KPStateMachine & sm) {
    auto & app = *static_cast<App *>(sm.controller);
    /*app.pump.off();
    app.shift.writeAllRegistersLow();
    app.intake.off();
    app.sensors.flow.stopMeasurement();*/

    app.vm.setValveStatus(app.status.currentValve, ValveStatus::sampled);
    app.vm.writeToDirectory();

    auto currentTaskId = app.currentTaskId;
    if(currentTaskId){
        app.tm.advanceTask(currentTaskId);
        app.tm.writeToDirectory();
    }
    app.currentTaskId       = 0;
    app.status.currentValve = -1;
    sm.next();
}

void TaskStateController::setup() {
    // registerState(SharedStates::Flush(), FLUSH1, [this](int code) {
    // 	switch (code) {
    // 	case 0:
    // 		return transitionTo(OFFSHOOT_CLEAN_1);
    // 	default:
    // 		halt(TRACE, "Unhandled state transition");
    // 	}
    // });
    // ..or alternatively if state only has one input and one output


    registerState(SharedStates::Sample(), SAMPLE, [this](int code) {
        //auto & app = *static_cast<App *>(controller);
        //app.sensors.flow.stopMeasurement();
        //app.logAfterSample();

        switch (code) {
        //Normal Exit
        case 0:
            return transitionTo(PRESERVE_FLUSH);
        //Pressure above system max, panic exit
        case -1:
            return transitionTo(STOP);
        default:
            halt(TRACE, "Unhandled state transition: ", code);
        }
    });

    registerState(SharedStates::PreserveFlush(), PRESERVE_FLUSH, [this](int code) {
        //Pressure above system max, panic exit
        if (code == -1)
            return transitionTo(STOP);
        return transitionTo(PRESERVE);
    });
    registerState(SharedStates::Preserve(), PRESERVE, [this](int code) {
        //Pressure above system max, panic exit
        if (code == -1)
            return transitionTo(STOP);
        return transitionTo(STOP);
    });

    registerState(Main::Stop(), STOP, IDLE);
    registerState(Main::Idle(), IDLE);
};