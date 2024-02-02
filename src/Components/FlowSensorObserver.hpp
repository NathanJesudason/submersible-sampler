#pragma once
#include <KPObserver.hpp>

class FlowSensorObserver : public KPObserver {
public:
    const char * ObserverName() const {
        return FlowSensorObserverName();
    }

    virtual const char * FlowSensorObserverName() const = 0;
    virtual void flowSensorDidUpdate(float f, float volume) {}
};
