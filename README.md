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

## Installation

[The latest release can be downloaded here.](https://github.com/clockspot/arduino-clock/releases) Please note [known bugs and to-dos.](https://github.com/clockspot/arduino-clock/blob/master/TODO.md)

### Code organization

The repository root doubles as both an Arduino IDE sketch folder and a PlatformIO project root:

* `arduino-clock.ino` is an empty marker — Arduino IDE only requires its presence (with a name matching the folder).
* `arduino-clock.h` is the central project header.
* `src/main.cpp` contains the actual sketch code (`setup()`, `loop()`, and most clock logic).
* `src/` also holds the module source files, grouped by hardware area:
  * `display*` — drivers for nixie tubes, MAX7219 LED matrices, and HT16K33 7-segment displays
  * `rtc*` — DS3231 hardware RTC and software (millis-based) backends
  * `input*` — `inputSimple` for button/rotary/IMU controls; `inputProton` for the Proton 320 clock radio retrofit
  * `network*` — Wi-Fi/NTP for Nano 33 IoT (NINA) and ESP32
  * `lightsensor*` — VEML7700 ambient light sensor
  * `storage` — persistent EEPROM/flash settings
  * `datetime` — pure date/time logic with no Arduino dependencies, covered by host-side unit tests
* `include/` holds the configuration files: `config.h`, `config.example.h`, and the `configs/` directory of hardware-specific configs and the `defaults.h` reference file.
* `test/` holds Unity tests for the host-side `native` PlatformIO environment.

Each module is conditionally compiled based on flags set in your config, so only the code for the hardware you've actually selected ends up in the binary.

### Configuration

Various options, such as enabled functionality, RTC, display, I/O pins, timeouts, and control behaviors, are specified in config files, which allow you to define configuration(s) to suit your particular clock's hardware.

The `include/configs/` folder includes many sample config files, as well as a `defaults.h` which shows all the possible configuration options with their defaults and full per-option documentation. You can create your own config files in a `custom/` folder that will be git-ignored. 

Config files don't need to specify every desired option — only the ones that differ from `defaults.h` (as this provides a default configuration). Each config must also explicitly pick one RTC type (e.g. `RTC_DS3231`) and one display type (e.g. `DISPLAY_NIXIE`).

To specify which configuration should be used at compile time, duplicate `include/config.example.h` as `include/config.h` and `#include` the desired config file. If you work with multiple clocks with different hardware profiles, you can use this file to easily switch between them by specifying multiple `#includes` and commenting out all but the relevant one.

You may also wish to adjust the defaults for the clock’s user-configurable values to best suit its intended use, in case the user performs a hard reset. Some of these are specified in the config; others, for now, are hardcoded in `src/main.cpp` (`optsDef[]` for [settings](https://github.com/clockspot/arduino-clock/blob/master/INSTRUCTIONS.md#settings-menu) and `initEEPROM()` for other values).

### Compilation and upload

The following libraries may be required, depending on the features enabled in the config:

* EEPROM (Arduino) for AVR Arduinos (e.g. classic Nano)
* SPI (Arduino) and [LedControl](http://wayoda.github.io/LedControl) for MAX7219-based matrix displays
* GFX and LEDBackpack (Adafruit) for HT16K33-based 7-segment displays
* VEML7700 (Adafruit) for VEML7700 ambient light sensor
* [Encoder](https://github.com/PaulStoffregen/Encoder) if rotary encoder is used for Up/Down inputs
* Arduino_LSM6DS3 (Arduino) if using Nano 33 IoT’s IMU for inputs
* WiFiNINA and WiFiUdp (Arduino) for Wi-Fi and NTP sync support on Nano 33 IoT
* [FlashStorage](https://github.com/cmaglie/FlashStorage/) for persistent storage on Nano 33 IoT
* Wire (Arduino) and RTClib (Adafruit) if using DS3231 RTC (via I2C)
* [Dusk2Dawn](https://github.com/dmkishi/Dusk2Dawn) if sunrise/sunset display is enabled
  * Note: At this writing, for Nano 33 IoT, it’s necessary to download this library as .ZIP and [add manually](https://www.arduino.cc/en/guide/libraries#toc4), as the version in the Library Manager [is old](https://forum.arduino.cc/index.php?topic=479550.msg3852574#msg3852574) and, in my experience, will not compile for SAMD.

Before compiling and uploading with the Arduino IDE, you will need to select the correct board, port, and (for AVR) processor in the Tools menu.

* If your Arduino does not appear as a port option, you may have a clone that requires [drivers for the CH340 chipset](https://sparks.gogo.co.nz/ch340.html).
* If upload fails for an ATMega328P Arduino (e.g. classic Nano), try selecting/unselecting “Old Bootloader” in the processor menu.

For PlatformIO, the `platformio.ini` at the repo root configures a host-side `native` environment for unit tests, plus commented-out stubs for hardware targets (`nano`, `nano_33_iot`, `esp32_s2`) — uncomment and adjust the matching one for your board. The `native` environment runs the [Unity](https://www.throwtheswitch.org/unity) tests under `test/` against the pure-logic modules (currently `datetime`); no hardware needed:

```
pio test -e native
```