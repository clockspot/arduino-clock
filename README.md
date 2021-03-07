# Universal Arduino Digital Clock

![Nixie clocks](https://i.imgur.com/FemMWax.jpg)

## Operating instructions

[The latest operating instructions (v2.0+) can be found here.](https://github.com/clockspot/arduino-clock/releases)

[Instructions for earlier versions are here.](https://github.com/clockspot/arduino-nixie/releases)

To see your clock’s software version, hold **Select** briefly while powering up the clock.

## About

**A universal digital clock codebase for Arduino.**

* Time of day with automatic DST change, display shutoff, and chimes.
* Perpetual calendar with day counter and local sunrise/sunset times.
* Alarm with snooze and automatic weekday/weekend skipping.
* Chronograph and timer with interval and reset options.
* Signals via piezo beeper, switch (appliance timer), pulse (bell ringer), and switchable backlighting.
* Supports Nixie displays of four/six digits, multiplexed in pairs via two SN74141 driver chips.
* Supports LED displays of three or four MAX7219 chips (via SPI) driving 8x8 LED matrices (example).
* Timekeeping can be internal, or based on a DS3231 RTC (via I2C) for reliability/accuracy.
* Supports both AVR and SAMD Arduinos (tested on Arduino Nano and Nano 33 IoT).
* On Nano 33 IoT, supports Wi-Fi connectivity with web-based config and NTP sync.

Originally written for [RLB Designs’](http://rlb-designs.com/) Universal Nixie Driver Board (UNDB):

* Backlighting (PWM LED) supported on UNDB v8+
* Switch and pulse signals supported on UNDB v9+
* Nano 33 IoT support coming on future versions

[The latest release can be downloaded here.](https://github.com/clockspot/arduino-nixie/releases/latest)

# Hardware configuration

A number of hardware-related settings are specified in a config file, so you can easily maintain multiple hardware profiles in their own config files, and include the relevant config at the top of the sketch before compiling. 

These settings include:

* **Number of digits** in the display module. Default is 6; small display adjustments are made for 4-tube clocks.
* **Which functions** are enabled (calendar, alarm, etc).
* **Which pins** are associated with the inputs (controls) and outputs (displays and signals).
  * If your clock includes LED backlighting (e.g. UNDB v8+), specifying an LED pin will enable the LED-related options in the options menu. LEDs should be connected to a PWM pin.
* **What type of Up/Down controls** are equipped: pushbuttons (default) or rotary encoder (TBD).
* **What type of signal outputs** are equipped: a piezo beeper (default) and/or a relay.
  * **Signal duration** (default 3min) and **piezo pulse duration** (default 500ms)
  * If your clock includes a relay (e.g. UNDB v9+), specifying a relay pin will enable the relay-related options in the options menu. You can also specify the **relay mode**:
    * In switched mode (default), the relay will be switched to control an appliance like a radio or lamp. If used with timer, it will switch on while timer is running (like the “sleep” function on a clock radio). If used with alarm, it will switch on when alarm trips and stay on for **relay switch duration** (default 2 hours). In this case, the **Alt** button (if equipped) will shut it off immediately, skipping snooze. This mode also enables the option for the LED backlighting, if equipped, to switch with the relay (great for a radio!).
    * In pulse mode, the relay will be pulsed, like the beeper is, to control an intermittent signaling device like a solenoid or indicator lamp; specify **relay pulse duration** (default 200ms).
* **Soft alarm switch** enabled: default is yes; it is switched with **Up** (on) and **Down** (off) while viewing the alarm time. Change to no if the signal output/appliance has its own switch on this relay circuit; the software alarm will be permanently on.
* **Soft power switch** enabled (switched relay only): default is yes; appliance can be toggled on/off with **Alt**. Change to no if the appliance has its own power switch (independent of this relay circuit) or does not need to be manually switched. (If set to no, or not using a switched relay, **Alt** acts as a function preset, as above.)
* **Various other durations** for things like scrolling speed, set function/display timeouts, short and long button holds, “hold to set faster” thresholds, etc.

You can also set the **defaults for the options menu** (in main code, currently) to better suit the clock’s intended use.

## Compiling the sketch

The author uses the Arduino IDE to compile, due to the use of various Arduino and Arduino-oriented libraries. Install them via the Library Manager as applicable (except where indicated otherwise).

* Wire (Arduino)
* EEPROM (Arduino) - for AVR-based Arduinos
* SPI (Arduino) and [LedControl](http://wayoda.github.io/LedControl) by Eberhard Farle - for MAX7219-based displays
* Arduino_LSM6DS3 (Arduino) - if using Nano 33 IoT's IMU (Inertial Measurement Unit) for inputs
* WiFiNINA and WiFiUdp (Arduino) - for Nano 33 IoT Wi-Fi and NTP sync support
* [DS3231](https://github.com/NorthernWidget/DS3231) by NorthernWidget - if using DS3231 for timekeeping
* [Dusk2Dawn](https://github.com/dmkishi/Dusk2Dawn) by DM Kishi - if sunrise/sunset display is enabled
  * Note: For Nano 33 IoT, it seems necessary to download the library from source as .ZIP and [add manually](https://www.arduino.cc/en/guide/libraries#toc4), rather than add directly via the Library Manager.
* [Encoder](https://github.com/PaulStoffregen/Encoder) by Paul Stoffregen - if rotary encoder is used for Up/Down inputs

**To upload the sketch to your clock,** if it doesn’t appear in the IDE’s Ports menu (as a USB port), your UNDB may be equipped with an Arduino clone that requires [drivers for the CH340 chipset](https://sparks.gogo.co.nz/ch340.html).