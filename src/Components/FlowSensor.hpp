//https://www.mouser.com/datasheet/2/698/REN_FS1025_DL_DST_20230503_2-2930723.pdf

#pragma once
#include <vector>
#include <KPSubject.hpp>
#include <Components/FlowSensorObserver.hpp>
#include <Wire.h>
#include "KellerLD.h"
#include <KPFoundation.hpp>

class FlowSensor : public KPComponent, public KPSubject<FlowSensorObserver> {

  public:
    const int ADDR = 0x50;
    FlowSensor(const char * name) : KPComponent(name) {}
    float flow;
    float volume;
    unsigned long updateTime;
    bool initialized;

    void setup() override {
      Wire.begin();
    }

    void resetVolume() {
      volume = 0.0;
    }

    
    float countToFlow(int count) {
        double ml_per_min = 0;
        if (count < 409) {
            ml_per_min = 0;
        } else if (count < 1558) {
            ml_per_min = (251.0/1558.0) * count;
        } else if (count < 1976) {
            ml_per_min = (513.0/1976.0)* count;
        } else if (count < 2214) {
            ml_per_min = (752.0/2214.0) * count;
        } else if (count < 2449) {
            ml_per_min = (1016.0/2449.0) * count;
        } else if (count < 2731) {
            ml_per_min = (1515.0/2731.0) * count;
        } else if (count < 2933) {
            ml_per_min = (2033.0/2933.0) * count;
        } else if (count < 3205) {
            ml_per_min =  (2977.0/3205.0) * count;
        } else if (count < 3346) {
            ml_per_min =  (3494.0/3346.0) * count;
        } else if (count < 3570) {
            ml_per_min =  (5045.0/3570.0) * count;
        } else if (count < 3650) {
            ml_per_min =  (6000.0/3650.0) * count;
        } else if (count < 3686) {
            ml_per_min =  (7000.0/3686.0) * count;
        }

        return ml_per_min;
    }

    float read() {
        // should return [Checksum, Flow HB, Flow LB, Generic Checksum, Generic Checksum]
        Wire.requestFrom(ADDR, 5, true);
        int checksum = Wire.read();
        int flow_h  = Wire.read();
        int flow_l  = Wire.read();
        int check_h   = Wire.read();
        int check_l   = Wire.read();

        //This should work, 256-modulo data and add with checksum for 0x00
        byte check = checksum + flow_h + flow_l + check_h + check_l;
        if (check) {
            println("flow sensor checksum failed!");
            return -1.0;
        }
        // mask high nibble from flow_h, then shift a byte and union with flow_l
        return {countToFlow(((flow_h & 0xF) << 8) | flow_l)};
    }

  void update() override {
    if ((!initialized) || (millis() < updateTime)){
      return;
    }
    updateTime = millis() + 1000;
    // Update pressure and temperature readings
    flow = read();

    //PressureSensorData data = {sensor.pressure(), sensor.temperature()};
    if(flow > 0){
      //flow of a second 
      volume += flow / 60.0;
      updateObservers(&FlowSensorObserver::flowSensorDidUpdate, flow, volume);
    }
  }

};