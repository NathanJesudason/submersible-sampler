// Manages Power and RTC

#pragma once 
#include <KPFoundation.hpp>
#include "RTClib.h"
#include <TimeLib.h>
//#include <LowPower.h>
#include <Wire.h>
#include <SPI.h>
#include <functional>

#include <Application/Constants.hpp>

extern volatile unsigned long rtcInterruptStart;
extern volatile bool alarmTriggered;
extern void rtc_isr();

class Power : public KPComponent {
  public:
    RTC_PCF8523 rtc;
    std::function<void()> interruptCallback;
    Power(const char * name) : KPComponent(name) {}

    void onInterrupt(std::function<void()> callbcak) {
        interruptCallback = callbcak;
    }

    void setupRTC() {
      rtc.begin();

      if (! rtc.initialized() || rtc.lostPower()){
        Serial.println("RTC is NOT initialized, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      }

      rtc.start();

      //TODO - add calibration from config vars here!
      //drift is between -64 and +63
      //float deviation_ppm = (drift / (7 * 86400 * 1000000));
      //int offset = round(deviation_ppm / 4.34);
      // rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration
      // rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct

      //Note: unable to sync provider because of the way RTC is defined in library, so we manually set resync time in App.hpp
      setTime(rtc.now().unixtime());
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  Print time to console formatted as YYYY/MM/DD hh:mm:ss GMT +/- offset
     *
     *  @param utc Unix epoch
     *  @param offset Offset in time zone
     *  ──────────────────────────────────────────────────────────────────────────── */
    void printTime(unsigned long utc, int offset = 0) {
        char message[64];
        utc = utc + (offset * 60 * 60);
        sprintf(message, "%u/%u/%u %02u:%02u:%02u GMT %+d", year(utc), month(utc), day(utc),
                hour(utc), minute(utc), second(utc), offset);
        print(message);
    }

    void printCurrentTime(int offset = 0) {
        printTime(rtc.now().unixtime(), offset);
    }

    void setup() override {
      setupRTC();
    }

    void update() override {
      //check for alarm trigger
        if (!alarmTriggered || !interruptCallback) {
            return;
        }

      //note: this assumes interrupt was read from rtc and not noise
      rtc.deconfigureAllTimers();
      noInterrupts();
      interruptCallback();
      alarmTriggered = false;
    }

    //CHECK FOR RELEVANCE/ACCURACY
    void sleepForever() {
        println();
        println("Going to sleep...");
        for (int i = 3; i > 0; i--) {
            print(F("-> "));
            println(i);
            delay(333);
        }

        //LowPower.standby();
        println();
        println("Just woke up due to interrupt!");
        printCurrentTime();
    }


    /** ────────────────────────────────────────────────────────────────────────────
     *  Set RTC time and internal timer of the Time library
     *
     *  @param seconds
     *  ──────────────────────────────────────────────────────────────────────────── */
    void set(unsigned long seconds) {
        println("Setting RTC Time...");
        printTime(seconds);

        setTime(seconds);  // Set time in Time library
        DateTime dt(seconds);
        rtc.adjust(dt);  // Set time for RTC
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  Schedule RTC alarm given TimeElements
     *
     *  @param future Must be in the future, otherwise this method does nothing
     *  ──────────────────────────────────────────────────────────────────────────── */
    void scheduleNextAlarm(TimeElements future) {
        unsigned long utc = makeTime(future);
        scheduleNextAlarm(utc);
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  Schedule RTC alarm given utc
     *
     *  @param utc Must be in the future, otherwise this method does nothing
     *  ──────────────────────────────────────────────────────────────────────────── */
    void scheduleNextAlarm(unsigned long utc) {
        unsigned long timestamp = now();
        if (utc < timestamp) {
            return;
        }

        println("Alarm triggering in: ", utc - timestamp, " seconds");
        setTimeout(utc - timestamp, true);
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  Put the chip into the low power state for specified number of seconds
     *
     *  @param seconds How long in seconds
     *  ────────────────────────────────────────────────────────────────────────────*/
    void sleepFor(unsigned long seconds) {
        setTimeout(seconds, true);
        sleepForever();
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  Schedule alarm for specified number of seconds from now
     *
     *  @param seconds How long until alarm
     *  @param usingInterrupt If true, the rtc fires interrupt at HardwarePins::RTC_INTERRUPT
     *  ────────────────────────────────────────────────────────────────────────────*/
    void setTimeout(unsigned long seconds, bool usingInterrupt) {
        TimeElements future;
        breakTime(rtc.now().unixtime() + seconds, future);
        rtc.deconfigureAllTimers();
        println("Setting alarm");

        //use second frequency if less than a 255 seconds
        if(seconds < 255)
          rtc.enableCountdownTimer(PCF8523_FrequencyHour, min((seconds), (unsigned long) 255));
        //Use minute frequency if time is less than 255 minustes
        if(seconds < 15300)
          rtc.enableCountdownTimer(PCF8523_FrequencyHour, min((seconds / 60), (unsigned long) 255));
        //Use hour frequency, clamp to 255 hours
        rtc.enableCountdownTimer(PCF8523_FrequencyHour, min((seconds / 3600), (unsigned long) 255));
        if (usingInterrupt) {
            println("Attaching Interrupt");
            attachInterrupt(digitalPinToInterrupt(HardwarePins::RTC_INTERRUPT), rtc_isr, FALLING);
        }
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  Convert compiled timestrings to seconds since 1 Jan 1970
     *
     *  @param offsetMinutes Minutes to offset time b (default: 0)
     *  @return time_t Time in seconds since 1 Jan 1970
     *  ──────────────────────────────────────────────────────────────────────────── */
    time_t compileTime(int offsetMinutes = 0) {
        const char * date   = __DATE__;
        const char * time   = __TIME__;
        const char * months = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char * m;

        // Get month from compiled date
        char month[4]{0};
        strncpy(month, date, 3);
        m = strstr(months, month);

        TimeElements tm;
        tm.Month  = ((m - months) / 3 + 1);
        tm.Day    = atoi(date + 4);
        tm.Year   = atoi(date + 7) - 1970;
        tm.Hour   = atoi(time);
        tm.Minute = atoi(time + 3);
        tm.Second = atoi(time + 6);

        time_t t = makeTime(tm);
        if (offsetMinutes) {
            t += offsetMinutes * 60;
            breakTime(t, tm);
        }

        // Add FUDGE factor to allow for time to compile
        constexpr time_t FUDGE = 10;
        return t + FUDGE;
    }
};