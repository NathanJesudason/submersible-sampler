#pragma once
#include <KPObserver.hpp>

class PressureSensorObserver : public KPObserver {
public:
    const char * ObserverName() const {
        return PressureSensorObserverName();
    }

    virtual const char * PressureSensorObserverName() const = 0;
    virtual void pressureSensorDidUpdate(float t, float p) {}
};
