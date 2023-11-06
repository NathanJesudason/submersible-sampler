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

    void setup() override {
      updateTime = millis();

      Wire.begin();

      // Initialize pressure sensor
      sensor.init();

      sensor.setFluidDensity(1029); // kg/m^3 (freshwater, 1029 for seawater)

      if(sensor.isInitialized()) {
        println("Sensor connected.");
      } else {
        println("Sensor not connected.");
      }
    }

  void update() override {
    if (millis() < updateTime)
      return;
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
    println(" m");

    print("Altitude: ");
    print(sensor.altitude());
    println(" m above mean sea level");

    delay(1000);*/
  }

};