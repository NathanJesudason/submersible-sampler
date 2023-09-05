#pragma once
#include <StateControllers/StateControllerBase.hpp>
#include <States/Shared.hpp>

namespace Task {
    STATE(IDLE);
    STATE(SAMPLE);
    STATE(PRESERVE_FLUSH);
    STATE(PRESERVE);
    STATE(STOP);

    struct Config {
        decltype(SharedStates::Sample::time) sampleTime;
        decltype(SharedStates::PreserveFlush::time) preserveDrawTime;
        decltype(SharedStates::Preserve::time) preserveTime;
    };

    class Controller : public StateController, public StateControllerConfig<Config> {
    public:
        Controller() : StateController("task-state-controller") {}

        void setup();
        void configureStates() {

            decltype(auto) sample = getState<SharedStates::Sample>(SAMPLE);
            sample.time           = config.sampleTime;

            decltype(auto) preserve_flush = getState<SharedStates::PreserveFlush>(PRESERVE_FLUSH);
            preserve_flush.time           = config.preserveDrawTime;

            decltype(auto) preserve = getState<SharedStates::Preserve>(PRESERVE);
            preserve.time           = config.preserveTime;
        }

        void begin() override {
            configureStates();
            transitionTo(SAMPLE);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }

        bool isStop() {
            return getCurrentState()->getName() == STOP;
        }
    };
};  // namespace Task

using TaskStateController = Task::Controller;
