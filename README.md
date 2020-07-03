# arduino-nixie

![Nixie clocks](https://i.imgur.com/FemMWax.jpg)

**A digital clock for the Arduino Nano and a nixie tube display.**

* Features perpetual calendar with day counter and sunrise/sunset, alarm with skip/snooze, and countdown timer.
* Supports automatic DST change, hourly chimes, LED lighting control, piezo beeper, and switchable relay.
* Supports four- or six-digit displays of Nixie tubes multiplexed in pairs via two SN74141 driver chips.
* Timekeeping requires a DS3231 real-time clock via I2C, which is battery-backed and thermocompensated.
* Written for [RLB Designs’](http://rlb-designs.com/) Universal Nixie Driver Board (UNDB), with LED control for v8+ and relay for v9+.

[The latest release can be downloaded here.](https://github.com/clockspot/arduino-nixie/releases/latest) Skip to [Hardware Configuration](#hardware-configuration) for details on tweaking the sketch.

## Operating Instructions, v1.6.0

When first powered up, the clock will display its software version (as of v1.6). [Instructions for earlier versions are here.](https://github.com/clockspot/arduino-nixie/releases)

Press **Select** to cycle through [Time of Day](#time-of-day), [Calendar](#calendar), [Alarm](#alarm), and [Countdown Timer](#countdown-timer).

To set something, simply hold **Select** until it flashes; use **Up/Down** to set, and **Select** to save.

### Time of Day

The time of day can be displayed in either 12h or 24h format per the [options menu](#options-menu), but when setting, it will display in 24h so you can tell AM from PM.

In the [options menu](#options-menu), you can also enable automatic daylight saving time adjustment, an hourly chime, and dimming/shutoff of the display at certain times, among other things.

### Calendar

The calendar cycles through several displays, before returning to the time of day:

* **The date.** Several formats are available in the [options menu](#options-menu). When setting, it will ask for year, then month, then date.

* **Day counter.** This will count down to, or up from, a date of your choice, repeating every year. When setting, it will ask for month, then date, then direction (0 = count down, 1 = count up).

  * TIP: To display the day of the year, set it to count up from December 31.

* **Sunrise and sunset.** These two displays show the previous and next sunrise or sunset (indicated by `1` or `0` on the seconds tubes). For this to work correctly, set your latitude, longitude, and UTC offset in the [options menu](#options-menu). They will display in the same 12h/24h format as the time of day.

### Alarm

The alarm is always shown in 24h format so you can tell AM from PM. Use **Up/Down** to switch the alarm between **on, skip, and off** (indicated by `1`/`01`/`0` on the seconds tubes, and/or high/medium/low beeps).

When the alarm sounds, press any button to snooze it, or briefly hold any button to silence it (it will give a short beep, and the display will blink once).

**Skip** will silence the next alarm in advance — useful if you’re taking a day off, or you wake up before your alarm. In the [options menu](#options-menu), you can program the alarm to skip automatically during the work week or on weekends – and when this is active, you can also _unskip_ the next alarm by simply switching it back on. The [Alt button](#the-alt-button) can be set to toggle the skip setting.

In the [options menu](#options-menu), you can set the snooze length and the alarm sound. If your clock has a [relay in switched mode](#hardware-configuration), you can also choose to switch on the relay at alarm time (like a clock radio) instead of sounding the beeper.

### Countdown Timer

The countdown timer can be set up to 18 hours. It begins running as soon as you finish setting it, and will continue to run in the background if you switch to a different display. To cancel the running timer, hold **Select** while the timer is displayed. When the timer runs out, press any button to silence. If power is lost, the timer will clear.

In the [options menu](#options-menu), you can set it to be an interval timer (restarting when it reaches zero), and can also select the timer sound. If your clock has a [relay in switched mode](#hardware-configuration), you can also choose to switch on the relay while the timer is running (like the “sleep” function on a clock radio) instead of sounding the beeper.

### The Alt Button

If your clock is equipped with an **Alt** button, it will do one of two things (depending on your [hardware configuration](#hardware-configuration)):

* If your clock has a switched relay with soft power switch enabled (such as for a radio), the **Alt** button acts as the power switch.

* Otherwise, it works as a preset button. While viewing the display you want quick access to (such as the alarm or countdown timer), hold **Alt** until it beeps twice; then you can use **Alt** to jump straight there.

  * TIP: If used with the alarm, the **Alt** button will also toggle the skip setting – so to skip (or unskip) the next alarm, you only need to press the **Alt** button twice: once to display it, and once to change it.

### Options Menu

* To enter the options menu, hold **Select** for 3 seconds until you see a single `1` on the hour tubes. This indicates option number 1.
* Use **Up/Down** to go to the option number you want to set (see table below); press **Select** to open it for setting (display will flash); use **Up/Down** to set; and **Select** to save.
* When all done, hold **Select** to exit the options menu.

|  | Option | Settings |
| --- | --- | --- |
|  | **General** |  |
| 1 | Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2 | Date format | 1 = month/date/weekday<br/>2 = date/month/weekday<br/>3 = month/date/year<br/>4 = date/month/year<br/>5 = year/month/date<br/>Note that four-tube clocks will display only the first two values in each of these options.<br/>The weekday is displayed as a number from 0 (Sunday) to 6 (Saturday). |
| 3 | Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date each minute at :30 seconds<br/>3 = same as 2, but scrolls in and out |
| 4 | Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |
| 5 | Digit fade | 0–20 (in hundredths of a second) |
| 6 | Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ)<br/>(If the clock is not powered at the time, it will correct itself when connected to power.) |
| 7 | LED behavior | 0 = always off<br/>1 = always on<br/>2 = on, but follow night/away shutoff if enabled<br/>3 = off, but on when alarm/timer sounds</br>4 = off, but on with switched relay (if equipped)<br/>(Clocks with LED lighting only) |
| 8 | Anti-cathode poisoning | Briefly cycles all digits to prevent [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm)<br/>0 = once a day, either at midnight or when night shutoff starts (if enabled)<br/>1 = at the top of every hour<br/>2 = at the top of every minute<br/>(Will not trigger during night/away shutoff) |
|  | **Alarm** |  |
| 10 | Alarm auto-skip | 0 = alarm triggers every day<br/>1 = work week only, skipping weekends (per settings below)<br/>2 = weekend only, skipping work week |
| 11 | Alarm signal | 0 = beeper<br/>1 = relay (if in switch mode, will stay on for 2 hours)<br/>(Clocks with both beeper and relay only) |
| 12 | Alarm beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 13 | Alarm snooze | 0–60 minutes. 0 disables snooze. |
|  | **Timer** |  |
| 20 | Timer interval mode | 0 = count down and stop<br/>1 = count down and restart (interval mode)<br/>(Clocks with beeper and/or pulse relay only) |
| 21 | Timer signal | 0 = beeper<br/>1 = relay (if in switch mode, will stay on until timer runs down)</br>(Clocks with both beeper and relay only) |
| 22 | Timer beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
|  | **Chime** |  |
| 30 | Chime | Make noise on the hour:<br/>0 = off<br/>1 = single beep<br/>2 = pips<br/>3 = Chime the hour (1 to 12)<br/>4 = ship’s bell (hour and half hour)<br/>Will not sound during night/away shutoff (except when off starts at top of hour)<br/>(Clocks with beeper or pulse relay only) |
| 31 | Chime signal | 0 = beeper<br/>1 = relay<br/>(Clocks with both beeper and pulse relay only) |
| 32 | Chime beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8). If using the pips, 63 (987 Hz) is closest to the real BBC pips frequency (1000 Hz).<br/>(Clocks with beeper only) |
|  | **Night/away shutoff** |  |
| 40 | Night shutoff | To save tube life and/or preserve your sleep, dim or shut off tubes nightly when you’re not around or sleeping.<br/>0 = none (tubes fully on at night)<br/>1 = dim tubes at night<br/>2 = shut off tubes at night<br/>When off, you can press **Select** to illuminate the tubes briefly. |
| 41 | Night starts at | Time of day. |
| 42 | Night ends at | Time of day. Set to 0:00 to use the alarm time. |
| 43 | Away shutoff | To further save tube life, shut off tubes during daytime hours when you’re not around. This feature is designed to accommodate your work schedule.<br/>0 = none (tubes on all day every day, except for night shutoff)<br/>1 = clock at work (shut off all day on weekends)<br/>2 = clock at home (shut off during work hours only)<br/>When off, you can press **Select** to illuminate the tubes briefly. |
| 44 | First day of work week | 0–6 (Sunday–Saturday) |
| 45 | Last day of work week | 0–6 (Sunday–Saturday) |
| 46 | Work starts at | Time of day. |
| 47 | Work ends at | Time of day. |
|  | **Geography** |  |
| 50 | Latitude | Your latitude, in tenths of a degree; negative (south) values are indicated with leading zeroes. (Example: Dallas is at 32.8°N, set as `328`.) |
| 51 | Longitude | Your longitude, in tenths of a degree; negative (west) values are indicated with leading zeroes. (Example: Dallas is at 96.7°W, set as `00967`.) |
| 52 | UTC offset | Your time zone’s offset from UTC (non-DST), in hours and minutes; negative (west) values are indicated with leading zeroes. (Example: Dallas is UTC–6, set as `0600`.) |

To reset the clock to “factory” defaults, hold **Select** while connecting the clock to power.

## Hardware Configuration

A number of hardware-related settings are specified in config files, so you can easily maintain multiple clocks with different hardware, by including the correct config file at the top of the sketch before compiling. 

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

### Compiling the sketch

**To compile the sketch,** ensure these libraries are added and enabled in your Arduino IDE, via the Library Manager:

* Wire (Arduino built-in)
* EEPROM (Arduino built-in)
* DS3231 ([NorthernWidget](https://github.com/NorthernWidget/DS3231))
* Dusk2Dawn ([dmkishi](https://github.com/dmkishi/Dusk2Dawn))

**To upload the sketch to your clock,** if it doesn’t appear in the IDE’s Ports menu (as a USB port), your UNDB may be equipped with an Arduino clone that requires [drivers for the CH340 chipset](https://sparks.gogo.co.nz/ch340.html).