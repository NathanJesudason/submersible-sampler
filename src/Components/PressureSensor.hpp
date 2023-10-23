#pragma once
#include <vector>
#include <KPSubject.hpp>
#include <Components/PressureSensorObserver.hpp>
#include <Wire.h>
#include "MS5837.h"
#include <KPFoundation.hpp>

class PressureSensor : public KPComponent, public KPSubject<PressureSensorObserver> {

  public:
    PressureSensor(const char * name) : KPComponent(name) {}
    MS5837 sensor;
    unsigned long updateTime;

    void setup() override {
      updateTime = millis();

      Wire.begin();

      // Initialize pressure sensor
      // Returns true if initialization was successful
      // We can't continue with the rest of the program unless we can initialize the sensor
      while (!sensor.init()) {
        println("Init failed!");
        println("Are SDA/SCL connected correctly?");
        println("Blue Robotics Bar30: White=SDA, Green=SCL");
        println("\n\n\n");
        delay(5000);
      }

      // .init sets the sensor model for us but we can override it if required.
      // Uncomment the next line to force the sensor model to the MS5837_30BA.
      //sensor.setModel(MS5837::MS5837_30BA);

      sensor.setFluidDensity(997); // kg/m^3 (freshwater, 1029 for seawater)
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