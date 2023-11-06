#pragma once
#include <vector>
#include <KPSubject.hpp>
#include <Components/PressureSensorObserver.hpp>
#include <Wire.h>
#include "KellerLD.h"
#include <KPFoundation.hpp>

class PressureSensor : public KPComponent, public KPSubject<PressureSensorObserver> {

  public:
    PressureSensor(const char * name) : KPComponent(name) {}
    KellerLD sensor;
    unsigned long updateTime;
    bool initialized;

    void setup() override {
      updateTime = millis() + 1000;

      Wire.begin();

      // Initialize pressure sensor
      sensor.init();

      sensor.setFluidDensity(1029); // kg/m^3 (freshwater, 1029 for seawater)

      initialized = sensor.isInitialized();
      if(initialized) {
        println("Sensor connected.");
      } else {
        println("Sensor not connected.");
        delay(1000);
      }
    }

  void update() override {
    if ((!initialized) || (millis() < updateTime)){
      return;
    }
    updateTime = millis() + 1000;
    // Update pressure and temperature readings
    sensor.read();

    //PressureSensorData data = {sensor.pressure(), sensor.temperature()};
    updateObservers(&PressureSensorObserver::pressureSensorDidUpdate, sensor.pressure(), sensor.temperature());

    print("Pressure: ");
    print(sensor.pressure());
    println(" mbar");

    print("Temperature: ");
    print(sensor.temperature());
    println(" deg C");

    /*print("Depth: ");
    print(sensor.depth());
    println(" m"); */
  }

};