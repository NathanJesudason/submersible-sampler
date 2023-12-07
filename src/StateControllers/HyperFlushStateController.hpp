#pragma once
#include <StateControllers/StateControllerBase.hpp>
#include <States/Shared.hpp>

namespace HyperFlush {
    STATE(IDLE);
    STATE(STOP);
    STATE(OFFSHOOT_PRELOAD);

    struct Config {
        decltype(SharedStates::Flush::time) flushTime;
        decltype(SharedStates::OffshootPreload::preloadTime) preloadTime;
    };

    class Controller : public StateControllerWithConfig<Config> {
    public:
        Controller() : StateControllerWithConfig("hyperflush-state-machine") {}

        //OFFSHOOT_PRELOAD -> STOP -> IDLE
        void setup() override {
            registerState(SharedStates::OffshootPreload(), OFFSHOOT_PRELOAD, STOP);
            registerState(SharedStates::Stop(), STOP, IDLE);
            registerState(SharedStates::Idle(), IDLE);
        }

        void begin() override {

            decltype(auto) preload = getState<SharedStates::OffshootPreload>(OFFSHOOT_PRELOAD);
            preload.preloadTime    = config.preloadTime;

            println("transitioning to offshoot preload");

            transitionTo(OFFSHOOT_PRELOAD);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }
    };
}  // namespace HyperFlush

using HyperFlushStateController = HyperFlush::Controller;
