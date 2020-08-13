# arduino-nixie

![Nixie clocks](https://i.imgur.com/FemMWax.jpg)

**A digital clock for the Arduino Nano and a nixie tube display.**

* Features perpetual calendar with day counter/sunrise/sunset, alarm with skip/snooze, and timer/chrono.
* Supports four- or six-digit displays of Nixie tubes multiplexed in pairs via two SN74141 driver chips.
* Can do auto DST change, tube shutoff, and chimes, and can control PWM LEDs, piezo beeper, and/or relay.
* Timekeeping requires a DS3231 real-time clock via I2C, which is battery-backed and thermocompensated.
* Written for [RLB Designs’](http://rlb-designs.com/) Universal Nixie Driver Board (UNDB), with LED control for v8+ and relay for v9+.

[The latest release can be downloaded here.](https://github.com/clockspot/arduino-nixie/releases/latest) Skip to [Hardware Configuration](#hardware-configuration) for details on tweaking the sketch.

# Operating instructions, v1.7.0

The clock displays its software version when powered up (as of v1.6). [Instructions for earlier versions are here.](https://github.com/clockspot/arduino-nixie/releases)

* Press **Select** to cycle through [time of day](#time-of-day), [calendar](#calendar), [alarm](#alarm), and [timer/chrono](#timer/chrono).
* To set anything, simply hold **Select** until the display flashes; use **Up/Down** to set, and **Select** to save. Additional settings are available in the [options menu](#options-menu).

## Time of day

The time of day is shown in 12h or 24h format per the [options menu](#options-menu), but when setting, it is shown in 24h so you can tell AM from PM. When exiting setting, seconds will reset to zero, unless the time was not changed.

## Calendar

The calendar cycles through several displays, before returning to the time of day:

* **The date.** Several formats are available in the [options menu](#options-menu). When setting, it will ask for the year, then the month, then the date.
* **Day counter.** This will count down to, or up from, a date of your choice, repeating every year. When setting, it will ask for the month, then the date, then the direction (0 = count down, 1 = count up).
  * TIP: To display the day of the year, set it to count up from December 31.
* **Sunrise/sunset.** These two displays show the previous and next apparent sunrise/sunset times (indicated by `1` or `0` on the seconds tubes – during the day, it shows sunrise then sunset; at night, sunset then sunrise). The times are calculated using the latitude, longitude, UTC offset, and auto DST rule specified in the [options menu](#options-menu), and shown in the same 12h/24h format as the time of day.
  * NOTE: At this writing, the times may be incorrect by a few minutes, depending on [your longitude and time of year](https://docs.google.com/spreadsheets/d/1dYchVCJAuhvosrCdtEeHLT3ZXcLZK8X0UtENItZR32M/edit#gid=0). I believe this to be a rounding error(s) in the [Dusk2Dawn library](https://github.com/dmkishi/Dusk2Dawn) (compared to the [NOAA Solar Calculator](https://www.esrl.noaa.gov/gmd/grad/solcalc/) it’s based on) and plan to investigate.

## Alarm

The alarm is always shown in 24h format so you can tell AM from PM.

* Use **Up/Down** to switch the alarm between **on, skip, and off** (indicated by `1`/`01`/`0` on the seconds tubes, and/or high/medium/low beeps).
  * If your clock has an **Alt** button and it’s [set as the alarm preset](#the-alt-button), it will switch the alarm as well – so you can display and switch the alarm with a few presses of a single button.
* **Skip** silences the next alarm in advance – useful if you’re taking a day off, or you wake up before your alarm. In the [options menu](#options-menu), you can program the alarm to skip automatically during the work week or on weekends – and when this is active, you can also _unskip_ the next alarm by simply switching it back on.
* When the alarm sounds, press any button – once to snooze, and again to cancel the snooze / silence the alarm for the day (it will give a short low beep, and the display will blink once).
  * In **Fibonacci mode** (see [options menu](#options-menu)), snooze does not take effect; any button press will silence the alarm for the day, even if the set alarm time hasn’t been reached yet.

## Timer/Chrono

This feature can count down (timer) or up (chrono) to 100 hours. When idle, it displays `0` (or `000000` if you have leading zeros enabled).

* To start and stop, use **Up**. While at `0`, this will start the chrono.
  * While the chrono is running, **Down** will briefly display a lap time.
* To set the timer, hold **Select** while at `0`. It will prompt for hours/minutes first, then seconds. For convenience, it will recall the last-used time. Once the timer is set, use **Up** to start it.
  * While the timer is running, **Down** will cycle through the runout options (what the timer will do when it runs out – clocks with beeper only):
    * 1 beep: simply stop, with a long signal (default)
    * 2 beeps: restart, with a short signal (makes a great interval timer!)
    * 3 beeps: start the chrono, with a long signal
    * 4 beeps: start the chrono, with a short signal
* To reset to `0`, hold **Select**.
  * It will automatically reset if you switch to a different function while it’s stopped, if it’s left stopped for an hour, if the chrono reaches 100 hours, or if power is lost. However, you can switch functions while it’s running, and it will continue to run in the background.
* When the timer signal sounds, press any button to silence it.

Some variations may apply, depending on your [hardware configuration](#hardware-configuration):

* If your clock has a switched relay and the timer/chrono is set to use it (in the [options menu](#options-menu)), it will switch on while the timer/chrono is running, like the “sleep” function on a clock radio.
* If your clock does not have a beeper, the runout options are not available.
* If your clock uses a rotary encoder for **Up/Down** rather than buttons, the controls are slightly different:
  * To start, use **Up**.
    * While the chrono is running, **Up** will briefly display a lap time.
    * While the timer is running, **Up** will cycle through the runout options.
  * To stop, use **Down**.

## The Alt button

If your clock has an **Alt** button, it will do one of two things (depending on your [hardware configuration](#hardware-configuration)):

* If your clock has a switched relay with soft power switch enabled (such as for a radio), **Alt** acts as that switch.
* Otherwise, it works as a preset button. While viewing the display you want quick access to (such as the alarm or timer/chrono), hold **Alt** until it beeps twice; then you can use **Alt** to jump straight there.
  * TIP: If **Alt** is set as the alarm preset, it will switch the alarm as well – so you can check and switch it with a few presses of a single button.

## Options menu

* To enter the options menu, hold **Select** for 3 seconds until you see a single `1` on the hour tubes. This indicates option number 1.
* Use **Up/Down** to go to the option number you want to set (see table below); press **Select** to open it for setting (display will flash); use **Up/Down** to set; and **Select** to save.
* When all done, hold **Select** to exit the options menu.

|  | Option | Settings |
| --- | --- | --- |
|  | **General** |  |
| 1 | Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2 | Date format | 1 = month/date/weekday<br/>2 = date/month/weekday<br/>3 = month/date/year<br/>4 = date/month/year<br/>5 = year/month/date<br/>The weekday is displayed as a number from 0 (Sunday) to 6 (Saturday).<br/>Four-tube clocks will display only the first two values in each of these options. |
| 3 | Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date each minute at :30 seconds<br/>3 = same as 2, but scrolls in and out |
| 4 | Leading zeros in hour, date/month, and timer/chrono? | 0 = no<br/>1 = yes |
| 5 | Digit fade | 0–20 (in hundredths of a second) |
| 6 | Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ)<br/>If the clock is not powered at the time, it will correct itself when powered up. |
| 7 | LED behavior | 0 = always off<br/>1 = always on<br/>2 = on, but follow night/away shutoff if enabled<br/>3 = off, but on when alarm/timer sounds</br>4 = off, but on with switched relay (if equipped)<br/>(Clocks with LED lighting only) |
| 8 | Anti-cathode poisoning | Briefly cycles all digits to prevent [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm)<br/>0 = once a day, either at midnight or when night shutoff starts (if enabled)<br/>1 = at the top of every hour<br/>2 = at the top of every minute<br/>(Will not trigger during night/away shutoff) |
|  | **Alarm** |  |
| 10 | Alarm auto-skip | 0 = alarm triggers every day<br/>1 = work week only, skipping weekends (per settings below)<br/>2 = weekend only, skipping work week |
| 11 | Alarm signal | 0 = beeper (uses pitch and pattern below)<br/>1 = relay (if in switch mode, will stay on for 2 hours)<br/>(Clocks with both beeper and relay only) |
| 12 | Alarm beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 13 | Alarm beeper pattern | 0 = long (1/2-second beep)<br/>1 = short (1/4-second beep)<br/>2 = double (two 1/8-second beeps)<br/>3 = triple (three 1/12-second beeps)<br/>4 = quad (four 1/16-second beeps)<br/>5 = cuckoo (two 1/8-second beeps, descending major third)<br/>(Clocks with beeper only) |
| 14 | Alarm snooze | 0–60 minutes. 0 disables snooze. |
| 15 | Fibonacci mode | 0 = off<br/>1 = on<br/>To wake you more gradually, starting about half an hour before the set time, the clock will beep at intervals per the [Fibonacci sequence](https://en.wikipedia.org/wiki/Fibonacci_number) (610 seconds, then 337, then 233...). In this mode, snooze does not take effect; any button press will silence the alarm for the day, even if the set alarm time hasn’t been reached yet.<br/>(Clocks with beeper and/or pulse relay only)
|  | **Timer/chrono** |  |
| 21 | Timer signal | 0 = beeper (uses pitch and pattern below)<br/>1 = relay (if in switch mode, will stay on until timer runs down)</br>(Clocks with both beeper and relay only) |
| 22 | Timer beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 23 | Timer beeper pattern | Same options as alarm beeper pattern.<br/>(Clocks with beeper only) |
|  | **Chime** |  |
| 30 | Chime | Make noise on the hour:<br/>0 = off<br/>1 = single pulse<br/>2 = [the pips](https://en.wikipedia.org/wiki/Greenwich_Time_Signal) (overrides pitch and pattern settings)<br/>3 = pulse the hour (1 to 12)<br/>4 = ship’s bell (hour and half hour)<br/>Will not sound during night/away shutoff (except when off starts at top of hour)<br/>(Clocks with beeper or pulse relay only) |
| 31 | Chime signal | 0 = beeper (uses pitch and pattern below)<br/>1 = relay<br/>(Clocks with both beeper and pulse relay only) |
| 32 | Chime beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 33 | Chime beeper pattern | Same options as alarm beeper pattern. Cuckoo recommended!<br/>(Clocks with beeper only) |
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

To reset the clock to “factory” defaults, hold **Select** while powering up the clock.

# Hardware configuration

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

## Compiling the sketch

**To compile the sketch,** ensure these libraries are added and enabled in your Arduino IDE, via the Library Manager:

* Wire (Arduino built-in)
* EEPROM (Arduino built-in)
* DS3231 ([NorthernWidget](https://github.com/NorthernWidget/DS3231))
* Dusk2Dawn ([dmkishi](https://github.com/dmkishi/Dusk2Dawn))

**To upload the sketch to your clock,** if it doesn’t appear in the IDE’s Ports menu (as a USB port), your UNDB may be equipped with an Arduino clone that requires [drivers for the CH340 chipset](https://sparks.gogo.co.nz/ch340.html).