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
|   5 | Retain data during restart    | RTC RAM        | a) Watchdog scratch registers<br>b) uninitialized RAM section <sup>1</sup>|

1) see [rp2_common/pico_platform/include/pico/platform.h: Section attribute macro for data that is to be left uninitialized](https://github.com/raspberrypi/pico-sdk/blob/6a7db34ff63345a7badec79ebea3aaef1712f374/src/rp2_common/pico_platform/include/pico/platform.h#L220)

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

## Notes

### General
- https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html
- https://datasheets.raspberrypi.com/rp2040/rp2040-product-brief.pdf
- https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
- https://github.com/raspberrypi/pico-sdk
- https://github.com/earlephilhower/arduino-pico
- https://arduino-pico.readthedocs.io/en/latest/index.html

### Adafruit Feather RP2040
- https://www.adafruit.com/product/4884
- https://learn.adafruit.com/adafruit-feather-rp2040-pico/

### Sleep Mode
  - Power-saving possibilities using the arduino-pico core
    https://github.com/earlephilhower/arduino-pico/discussions/1544#discussioncomment-6205182
    - https://github.com/lyusupov/SoftRF/tree/master/software/firmware/source/libraries/RP2040_Sleep
      - https://github.com/raspberrypi/pico-extras/blob/master/src/rp2_common/pico_sleep/sleep.c
      - https://github.com/raspberrypi/pico-extras/blob/master/src/rp2_common/pico_sleep/include/pico/sleep.h
      - https://github.com/raspberrypi/pico-extras/blob/master/src/rp2_common/hardware_rosc/rosc.c
      - https://github.com/raspberrypi/pico-extras/blob/master/src/rp2_common/hardware_rosc/include/hardware/rosc.h
  - https://github.com/earlephilhower/arduino-pico/issues/345
  - https://www.heise.de/blog/Sleepy-Pico-ein-Raspberry-Pi-Pico-geht-mit-C-C-schlafen-6046517.html
  - Software Reset: https://forums.raspberrypi.com/viewtopic.php?t=318747
  - https://github.com/raspberrypi/pico-extras/blob/master/src/rp2_common/pico_sleep/sleep.c
  - void rp2040.reboot()
    https://github.com/earlephilhower/arduino-pico/blob/master/docs/rp2040.rst#void-rp2040reboot
  - RP2040 ultra low power wake on RTC: https://forums.raspberrypi.com/viewtopic.php?t=338245
  - https://ghubcoder.github.io/posts/awaking-the-pico/

### Resets
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2_common/hardware_resets/include/hardware/resets.h
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2040/hardware_regs/include/hardware/regs/resets.h
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2040/hardware_structs/include/hardware/structs/vreg_and_chip_reset.h
  - Arduino Nano RP2040 Connect - determine reason for reset / read rp2040 chip_reset register - SOLVED - Nano Family / Nano RP2040 Connect - Arduino Forum
  - restart()/reboot(): 
    - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/cores/rp2040/RP2040Support.h
    - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2_common/hardware_watchdog/watchdog.c
    - bool watchdog_caused_reboot(void); 
      - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2_common/hardware_watchdog/include/hardware/watchdog.h

### RTC
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_rtc
  - Setting/getting RTC time
    https://github.com/raspberrypi/pico-examples/blob/master/rtc/hello_rtc/hello_rtc.c
  - ESP32Time:
    - rtc.setTime(time_t)
    - time_t rtc.getLocalEpoch()
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2040/hardware_structs/include/hardware/structs/rtc.h
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2040/hardware_regs/include/hardware/regs/rtc.h
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/pico-sdk/src/rp2_common/hardware_rtc/rtc.c
  - https://community.element14.com/products/arduino/b/blog/posts/real-time-clock-on-the-raspberry-pi-pico
  - ~/.arduino15/packages/rp2040/hardware/rp2040/3.6.0/libraries/rp2040/examples/Time/Time.ino
  - Is the RTC nearly useless or is it me?
    https://forums.raspberrypi.com/viewtopic.php?t=325598#p1950784
  - no separate RTC XTAL on Adafruit Feather RP2040
  - Using Timezones/Local Time on the Pico
    https://forums.raspberrypi.com/viewtopic.php?t=350488
