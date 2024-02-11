// https://sensirion.com/media/documents/5D69DA5A/6387516F/Sensirion_Liquid_Flow_Sensors_Datasheet_SLF3S-4000B.pdf 

#pragma once
#include <vector>
#include <KPSubject.hpp>
#include <Components/FlowSensorObserver.hpp>
#include <Wire.h>
#include <SensirionI2cSf06Lf.h>
#include <KPFoundation.hpp>

class FlowSensor : public KPComponent, public KPSubject<FlowSensorObserver> {

  //Addr will have to change, because there is flow sensor for each line
  //Add in code for multiplexer, 3 in total for up to 24 https://www.adafruit.com/product/2717
  public:
  //we select which flow sensor to use based on the SC pin and addr of the multiplexer
    const int multiplexerADDR = 0x70;
    SensirionI2cSf06Lf sensor;
    FlowSensor(const char * name) : KPComponent(name) {}
    float flow = 0.0;
    float volume;
    unsigned long updateTime;
    bool initialized;

    void setup() override {
      Wire.begin();
      sensor.begin(Wire, SLF3S_4000B_I2C_ADDR_08);
      sensor.stopContinuousMeasurement();
      delay(100);
    }

    void startMeasurement(int valvePin) {
      //valvePin corresponds to the different flow sensors
      int multiplexerOffset = valvePin / 8;
      Wire.beginTransmission(multiplexerADDR + multiplexerOffset);
      Wire.write(1 << (valvePin % 8));
      Wire.endTransmission();
      volume = 0.0;
      initialized = true;
      sensor.startH2oContinuousMeasurement();
    }

    void endMeasurement(){
      initialized = false;
      sensor.stopContinuousMeasurement();
    }

  void update() override {
    if ((!initialized) || (millis() < updateTime)){
      return;
    }
    updateTime = millis() + 500;
    // Update pressure and temperature readings

    float aTemperature = 0.0;
    uint16_t aSignalingFlags = 0u;
    int16_t error;

    error = sensor.readMeasurementData(INV_FLOW_SCALE_FACTORS_SLF3S_4000B, flow, aTemperature, aSignalingFlags);


    //PressureSensorData data = {sensor.pressure(), sensor.temperature()};
    if(error != NO_ERROR){
      //flow of a second 
      volume += flow / 120.0;
      updateObservers(&FlowSensorObserver::flowSensorDidUpdate, flow, volume);
    }
  }

};