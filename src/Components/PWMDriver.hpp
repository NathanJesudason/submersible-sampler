#pragma once
#include <KPFoundation.hpp>
#include <Components/PumpStatus.hpp>
#include <Adafruit_PWMServoDriver.h>


class PWMDriver : public KPComponent {
  public:
    const int capacityPerDriver = 16;
    int8_t* pumps;
    int pumpsCount;
    Adafruit_PWMServoDriver drives[2] = {Adafruit_PWMServoDriver(0x40), Adafruit_PWMServoDriver(0x41)};
    PWMDriver(const char * name, int pumpCount) : KPComponent(name), pumpsCount(pumpCount) {
      pumps = new int8_t[pumpCount]();
    }
    void setup() override {
      drives[0].setPWMFreq(1600);
      drives[1].setPWMFreq(1600);
      writeAllPumpsOff();
    }

    void writeAllPumpsOff(){
      for(int i = 0; i < pumpsCount; i++){
        drives[i / capacityPerDriver].writeMicroseconds(i % capacityPerDriver, 1500);
        pumps[i] = PumpStatus::off;
      }
    }

    void writeAllPumps(){
      for(int i = 0; i < pumpsCount; i++){
        if(pumps[i] == PumpStatus::off)
          drives[i / capacityPerDriver].writeMicroseconds(i % capacityPerDriver, 1500);
        else if (pumps[i] == PumpStatus::forwards)
          drives[i / capacityPerDriver].writeMicroseconds(i % capacityPerDriver, 1900);
        else
          drives[i / capacityPerDriver].writeMicroseconds(i % capacityPerDriver, 1100);
      }
    }

    void writePump(int pump, PumpStatus signal){
      if(pump < 0 || pump >= pumpsCount)
        return;
      pumps[pump] = signal;
      if(pumps[pump] == PumpStatus::off)
        drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, 1500);
      else if (pumps[pump] == PumpStatus::forwards)
        drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, 1900);
      else
        drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, 1100);
    }

};