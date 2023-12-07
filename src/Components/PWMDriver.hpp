#pragma once
#include <KPFoundation.hpp>
#include <Components/PumpStatus.hpp>
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>


class PWMDriver : public KPComponent {
  public:
    const int capacityPerDriver = 16;
    int8_t* pumps;
    int pumpsCount;
    //Adafruit_PWMServoDriver driver = Adafruit_PWMServoDriver();
    Adafruit_PWMServoDriver drives[2] = {Adafruit_PWMServoDriver(0x41), Adafruit_PWMServoDriver(0x42)};
    PWMDriver(const char * name, int pumpCount) : KPComponent(name), pumpsCount(pumpCount) {
      pumps = new int8_t[pumpCount]();
    }
    void setup() override {
      //driver.begin();
      drives[0].begin();
      drives[1].begin();
      println("setting pwm");
      drives[0].setOscillatorFrequency(25000000);
      drives[1].setOscillatorFrequency(27000000);
      drives[0].setPWMFreq(200);
      drives[1].setPWMFreq(200);
      delay(10);
      //drives[1].setPWMFreq(1600);
      println("setting pumps off");
      writeAllPumpsOff();
      delay(5000);
      println("done");
    }

    void writeAllPumpsOff(){
      for(int i = 0; i < pumpsCount; i++){
        drives[i / capacityPerDriver].writeMicroseconds(i % capacityPerDriver, 1500);
        pumps[i] = PumpStatus::off;
      }
    }

    void writePump(int pump, PumpStatus signal){
      if(pump < 0 || pump >= pumpsCount)
        return;
      //Temporarily reducing forwards and backwords strength
      //https://bluerobotics.com/wp-content/uploads/2022/04/thruster-usage-guide-PWM-signal.png 
      pumps[pump] = signal;
      if(pumps[pump] == PumpStatus::off)
        drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, 1500);
      else if (pumps[pump] == PumpStatus::forwards)
        drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, 1800);
      else
        drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, 1400);
    }


    void writePumpVariable(int pump, int speed){
      if(pump < 0 || pump >= pumpsCount)
        return;
      if((speed > 1900) || (speed < 1100))
        return;
      if(speed > 1500)
        pumps[pump] = PumpStatus::forwards;
      else if (speed < 1500)
        pumps[pump] = PumpStatus::backwards;
      else
        pumps[pump] = PumpStatus::off;
      drives[pump / capacityPerDriver].writeMicroseconds(pump % capacityPerDriver, speed);
    }

};