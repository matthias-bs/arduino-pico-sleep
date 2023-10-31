# arduino-pico-sleep
Example sketch for using sleep mode (wake-up by RTC) with Arduino Pico RP2040

This example allows to implement a sleep function similar to the ESP32's.
See the differences below:

| No. | Topic                         | ESP32          | RP2040                     |
| --- | ----------------------------- | -------------- | -------------------------- |
|   1 | Wake-up criterion             | Sleep duration | Wake-up date & time        |  
|   2 | Wake-up behaviour             | Restart sketch | Continue sketch            |
|   3 | RTC reset by restart          | No             | Yes                        |
|   4 | Retain data during sleep mode | RTC RAM        | Yes                        |
|   5 | Retain data during restart    | RTC RAM        | Watchdog scratch registers |


## RP2040 Workarounds/Solutions

1. Create date and time from epoch, set HW RTC
   Instead of using `struct tm`, `datetime_t` has to be used for this.

2. If the sketch needs to be restartet after wake-up, `rp2040.restart()` has to be called.

3. A restart resets the RTC. This is a problem if you are using the RTC for providing time and date.

4. The RP2040 retains all data during sleep mode - nothing to be done here.

5. If you only have to keep a few bytes during restart, you can use the watchdog scratch registers. Please note that the bootloader used a few of them too.
   In the example, the epoch time (variable `time_saved`) and a few application variables are retained this way.

## RTC Setting / Resynchronization

In an embedded device, you would normally use
* Internet (NTP)
* A radio clock signal
* GPS
* LoRaWAN
as the time source for setting the RTC.

In this example, the user is requested to enter date/time via serial console.

Embedded RTCs (or more precisely: their crystal oscillators) tend to have certain inaccuracies which accumulate over time. Therefore it is often desired to resynchronize the RTC to the time source after a certain time (`CLOCK_SYNC_INTERVAL`). For this purpose, the variable `rtcLastClockSync` is used to keep the timestamp of the last synchronization.


## Extras

In some cases, it is useful to align the wake-up time to the wall clock time, e.g. at 5, 10, 15 minutes etc. after the full hour. This should even work with varying run-time after wake-up. The example provides this, too.
