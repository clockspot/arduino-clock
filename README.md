# Universal Arduino Digital Clock

![Arduino clocks](https://theclockspot.com/arduino-clock.jpg)

## Operating instructions

[The latest operating instructions (v2.0+) can be found here.](https://github.com/clockspot/arduino-clock/blob/master/INSTRUCTIONS.md)

[Instructions for earlier versions are here.](https://github.com/clockspot/arduino-clock/releases)

To see your clock’s software version, hold **Select** briefly while powering up the clock.

## About

**A universal digital clock codebase for Arduino,** maintained by [Luke](https://theclockspot.com).

* **Time of day** with automatic DST change and chimes.
* **Perpetual calendar** with day counter and local sunrise/sunset times.
* **Alarm** with snooze and automatic weekday/weekend skipping.
* **Chronograph and timer** with reset/interval options.
* Display types supported:
  * **Nixie displays** of four/six tubes, driven by two SN74141 chips, with anti-cathode poisoning.
  * **LED matrices** of 8x24/8x32 LEDs, driven by three/four SPI MAX7219 chips ([example](https://www.amazon.com/HiLetgo-MAX7219-Arduino-Microcontroller-Display/dp/B07FFV537V/)).
  * **LED 7-segment displays** of four/six digits, driven by an I2C HT16K33 ([example](https://learn.adafruit.com/adafruit-led-backpack/1-2-inch-7-segment-backpack)).
* **Display dimming/shutoff** on nightly/weekly schedule or per ambient lighting via [I2C VEML7700 sensor](https://learn.adafruit.com/adafruit-veml7700).
* **Switchable backlighting** (single-channel) with optional PWM fade.
* **Simple control** via three/four buttons, a rotary encoder, and/or Nano 33 IoT’s [IMU](https://en.wikipedia.org/wiki/Inertial_measurement_unit) (tilt control).
* **Signals** via piezo beeper, switch (e.g. appliance timer), and/or pulse (e.g. bell ringer).
* Tested on both classic Arduino Nano (AVR) and Nano 33 IoT (SAMD21).
* Supports **web-based config and NTP sync** over Wi-Fi on Nano 33 IoT.
* **Timekeeping** can be internal, or based on an I2C DS3231 RTC for reliability/accuracy.
* Settings stored persistently in case of power loss, and mirrored in RAM in case of EEPROM/flash failure.

Written to support [RLB Designs’](http://rlb-designs.com/) Universal Nixie Driver Board (UNDB):

* Backlighting (PWM LED) supported on UNDB v8+
* Switch and pulse signals supported on UNDB v9+
* Nano 33 IoT support coming on future versions

[The latest release can be downloaded here.](https://github.com/clockspot/arduino-clock/releases) Please note [known bugs and to-dos.](https://github.com/clockspot/arduino-clock/blob/master/TODO.md)

# Configuration, compilation, and upload

Various options, such as enabled functionality, RTC, display, I/O pins, timeouts, and control behaviors, are specified in a config file. This allows you to maintain configs for multiple clock hardware profiles, and simply include the relevant config at the top of `arduino-clock.h` before compiling. Several [example configs](https://github.com/clockspot/arduino-clock/tree/master/arduino-clock/configs) are provided, and [`~sample.h`](https://github.com/clockspot/arduino-clock/blob/master/arduino-clock/configs/%7Esample.h) includes all possible options with detailed comments.

You may also wish to adjust the defaults for the clock’s user-configurable values to best suit its intended use, in case the user performs a hard reset. Some of these are specified in the config; others, for now, are hardcoded in `arduino-clock.ino` (`optsDef[]` for [settings](https://github.com/clockspot/arduino-clock/blob/master/INSTRUCTIONS.md#settings-menu) and `initEEPROM()` for other values).

I use the Arduino IDE to compile and upload, due to the use of various Arduino and Arduino-oriented libraries. Make sure the relevant libraries are installed in the Library Manager, per the config in use.

* EEPROM (Arduino) for AVR Arduinos (e.g. classic Nano)
* SPI (Ardunio) and [LedControl](http://wayoda.github.io/LedControl) for MAX7219-based matrix displays
* GFX and LEDBackpack (Adafruit) for HT16K33-based 7-segment displays
* VEML7700 (Adafruit) for VEML7700 ambient light sensor
* [Encoder](https://github.com/PaulStoffregen/Encoder) if rotary encoder is used for Up/Down inputs
* Arduino_LSM6DS3 (Arduino) if using Nano 33 IoT’s IMU for inputs
* WiFiNINA and WiFiUdp (Arduino) for Wi-Fi and NTP sync support on Nano 33 IoT
* [FlashStorage](https://github.com/cmaglie/FlashStorage/) for persistent storage on Nano 33 IoT
* Wire (Arduino) and RTClib (Adafruit) if using DS3231 RTC (via I2C)
* [Dusk2Dawn](https://github.com/dmkishi/Dusk2Dawn) if sunrise/sunset display is enabled
  * Note: At this writing, for Nano 33 IoT, it’s necessary to download this library as .ZIP and [add manually](https://www.arduino.cc/en/guide/libraries#toc4), as the version in the Library Manager [is old](https://forum.arduino.cc/index.php?topic=479550.msg3852574#msg3852574) and, in my experience, will not compile for SAMD.

Before compiling and uploading, you will need to select the correct board, port, and (for AVR) processor in the IDE’s Tools menu.

* If your Arduino does not appear as a port option, you may have a clone that requires [drivers for the CH340 chipset](https://sparks.gogo.co.nz/ch340.html).
* If upload fails for an ATMega328P Arduino (e.g. classic Nano), try selecting/unselecting “Old Bootloader” in the processor menu.