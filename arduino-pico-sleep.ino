///////////////////////////////////////////////////////////////////////////////
// arduino-pico-sleep.ino
// 
// Example for using sleep mode with wake-up by RTC similar to ESP32.
//
// - Allows to specify sleep duration (instead of RP2040' wake-up date & time)
// - Optional restart after wake-up
// - Retention of some variables and RTC time during restart 
//
// Based on:
// ---------
// ESP32Time by Felix Biego (https://github.com/fbiego/ESP32Time) 
//
// Library dependencies (tested versions):
// ---------------------------------------
// ESP32Time                            2.0.4
//
// created: 10/2023
//
//
// MIT License
//
// Copyright (c) 2023 Matthias Prinke
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
// History:
//
// 20231031 Created from BresserWeatherSensorTTN
//
// Notes:
// - settimeofday()/gettimeofday() must be used to access the ESP32's RTC time
// - Arduino ESP32 package has built-in time zone handling, see 
//   https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
//   This is also provided by Arduino-pico.
//
///////////////////////////////////////////////////////////////////////////////
/*! \file arduino-pico-sleep.ino */ 

#include <Arduino.h>
#include <ESP32Time.h>

#ifdef ARDUINO_ARCH_RP2040
    #include "src/pico_rtc/pico_rtc_utils.h"
    #include <hardware/rtc.h>
#endif

// Your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

// RTC sync interval (in minutes)
#define CLOCK_SYNC_INTERVAL 10 // 24 * 60

// MCU will sleep for SLEEP_INTERVAL seconds
#define SLEEP_INTERVAL 120

// Set to mimic ESP32's bahaviour
#define REBOOT_AFTER_WAKEUP

// Variables which must retain their values after deep sleep
#if defined(ESP32)
    // Stored in RTC RAM
    RTC_DATA_ATTR bool      runtimeExpired = false;   //!< flag indicating if runtime has expired at least once 
    RTC_DATA_ATTR bool      longSleep;                //!< last sleep interval; 0 - normal / 1 - long
    RTC_DATA_ATTR time_t    rtcLastClockSync = 0;     //!< timestamp of last RTC synchonization to network time
#else
    // Save to/restored from Watchdog SCRATCH registers
    bool                    runtimeExpired;           //!< flag indicating if runtime has expired at least once
    bool                    longSleep;                //!< last sleep interval; 0 - normal / 1 - long
    time_t                  rtcLastClockSync;         //!< timestamp of last RTC synchonization to network time
#endif

/// RTC sync request flag - set (if due) in setup() / cleared in UserRequestNetworkTimeCb()
bool rtcSyncReq = false;

/// Real time clock
ESP32Time rtc;

/// Print date and time (i.e. local time)
void printDateTime(void) {
        struct tm timeinfo;
        char tbuf[25];
        
        time_t tnow = rtc.getLocalEpoch();
        localtime_r(&tnow, &timeinfo);
        strftime(tbuf, 25, "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("%s\n", tbuf);
}




/// Arduino setup
void setup() {
    #if defined(ARDUINO_ARCH_RP2040)
        // see pico-sdk/src/rp2_common/hardware_rtc/rtc.c
        rtc_init();

        // Restore variables and RTC after reset 
        time_t time_saved = watchdog_hw->scratch[0];
        datetime_t dt;
        epoch_to_datetime(&time_saved, &dt);
        
        // Set HW clock (only used in sleep mode)
        rtc_set_datetime(&dt);
        
        // Set SW clock
        rtc.setTime(time_saved);

        runtimeExpired   = ((watchdog_hw->scratch[1] & 1) == 1);
        longSleep        = ((watchdog_hw->scratch[1] & 2) == 2);
        rtcLastClockSync = watchdog_hw->scratch[2];
    #endif

    // set baud rate
    Serial.begin(115200);
    delay(3000);
    Serial.setDebugOutput(true);

    // wait for serial to be ready - or timeout if USB is not connected
    delay(500);

    // Set time zone
    setenv("TZ", TZ_INFO, 1);
    Serial.printf("\n\nRestarted:\n");
    printDateTime();
    
    // Check if clock was never synchronized or sync interval has expired 
    if ((rtcLastClockSync == 0) || ((rtc.getLocalEpoch() - rtcLastClockSync) > (CLOCK_SYNC_INTERVAL * 60))) {
        Serial.print("RTC sync required\n");
        rtcSyncReq = true;
    }

    if (rtcSyncReq) {
        // Synchronize RTC to time source
        // You would normally use NTP, GPS, a radio clock signal or LoRaWAN here...

        // Read string from serial console
        Serial.printf("Enter time in the format YYYY-MM-DD HH:MM\n?\n");
        while (!Serial.available());
        String time_str = Serial.readStringUntil('\n');

        // Convert string to struct tm
        struct tm ti;
        // strptime() is not available...
        // strptime(buf, "%Y-%d-%m %H:%M", &ti)
        ti.tm_isdst = -1;
        int year;
        int month;
        sscanf(time_str.c_str(), "%d-%d-%d %d:%d", &year, &month, &ti.tm_mday, &ti.tm_hour, &ti.tm_min);
        ti.tm_year = year - 1900;
        ti.tm_mon = month - 1;
        
        // Print result for checking
        char tbuf[25];
        strftime(tbuf, 25, "%Y-%m-%d %H:%M:%S", &ti);
        Serial.printf("Parsed time string: %s\n", tbuf);

        // Set SW clock
        time_t now = mktime(&ti);
        rtc.setTime(now);

        // Save last 
        rtcLastClockSync = now;
    }
    prepareSleep();
}


/// Arduino execution loop
void loop() {

}


/// Determine sleep duration and enter Deep Sleep Mode
void prepareSleep(void) {
    unsigned long sleep_interval = SLEEP_INTERVAL;

    // If the real time is available, align the wake-up time to the
    // next non-fractional multiple of sleep_interval past the hour
    if (rtcLastClockSync) {
        struct tm timeinfo;
        time_t t_now = rtc.getLocalEpoch();
        localtime_r(&t_now, &timeinfo);        
        sleep_interval = sleep_interval - ((timeinfo.tm_min * 60) % sleep_interval + timeinfo.tm_sec);
        if (sleep_interval < 60) {
            sleep_interval = 60;
        }
    }
    
    Serial.printf("Shutdown() - sleeping for %lu s\n", sleep_interval);
    #if defined(ESP32)
        SLEEP_INTERVAL += 20; // Added extra 20-secs of sleep to allow for slow ESP32 RTC timers
        ESP.deepSleep(sleep_interval * 1000000LL);
    #else
        // Set HW clock from SW clock
        time_t t_now = rtc.getLocalEpoch();
        datetime_t dt;
        epoch_to_datetime(&t_now, &dt);
        rtc_set_datetime(&dt);
        sleep_us(64);
        pico_sleep(sleep_interval);

        // Save variables to be retained after reset
        watchdog_hw->scratch[2] = rtcLastClockSync;
        
        if (runtimeExpired) {
            watchdog_hw->scratch[1] |= 1;
        } else {
            watchdog_hw->scratch[1] &= ~1;
        }
        if (longSleep) {
            watchdog_hw->scratch[1] |= 2;
        } else {
            watchdog_hw->scratch[1] &= ~2;
        }

        // Save the current time, because RTC will be reset (SIC!)
        rtc_get_datetime(&dt);
        time_t now = datetime_to_epoch(&dt, NULL);
        watchdog_hw->scratch[0] = now;
        //Serial.printf("Now: %llu\n", now);
        
        #if defined(REBOOT_AFTER_WAKEUP)
        rp2040.restart();
        #endif
    #endif
}
