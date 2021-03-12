# Operating instructions, v2.0+

> [Instructions for earlier versions are here.](https://github.com/clockspot/arduino-clock/releases) To see your clock’s software version, hold **Select** briefly while powering up the clock.

Your clock has four main functions: [time of day](#time-of-day), [calendar](#calendar), [alarm](#alarm), and [chrono/timer](#chronotimer). To cycle through these functions, press **Select**.

To set anything, simply hold **Select** until the display blinks; use **Up/Down** to set, and **Select** to save.

If your clock has an **Alt** button, it will act as a [function preset](#function-preset), [power button](#signals), and/or [Wi-Fi button](#wi-fi-support), depending on your clock’s features.

Additional settings are available in the [settings menu](#settings-menu). If you have a Wi-Fi-enabled clock, you can configure these settings (and more) on the [settings page](#wi-fi-support), where you can also configure the clock to set itself.

## Time of day

The time of day can be [set](#settings-menu) to display in 12h or 24h format.

* When setting, it is shown in 24h so you can tell AM from PM; and when the time is changed, the seconds will reset to zero.

## Calendar

The calendar cycles through several displays, before returning to the time of day:

* **The date.** Several formats are available in the [settings menu](#settings-menu).
  * When setting, it will ask for the year, then the month, then the date.
* **Day counter.** This will count down to, or up from, a date of your choice, repeating every year.
  * When setting, it will ask for the month, then the date, then the direction (0 = count down, 1 = count up).
  * TIP: To display the day of the year, set it to count up from December 31.
  * To disable the day counter (Wi-Fi clocks only), use the [settings page](#wi-fi-support).
* **Sunrise/sunset.** These two displays show the previous and next apparent sunrise/sunset times (indicated by `1` or `0` in place of seconds – during the day, it shows sunrise then sunset; at night, sunset then sunrise), in the same 12h/24h format as the time of day.
  * Specify your latitude, longitude, and UTC offset in settings.
  * To disable sunrise/sunset, set latitude/longitude to 0.
* _Future software versions will add support for weather forecasts for Wi-Fi-enabled clocks._

## Alarm

The alarm is always shown in 24h format so you can tell AM from PM. In place of seconds, it displays `1`/`01`/`0` to indicate **on, skip, and off,** and the display dims when the alarm is off.

* Use **Up/Down** to switch the alarm between **on, skip, and off,** indicated by high/medium/low beeps.
	* **Skip** silences the next alarm in advance (hence the `01` meaning “off then on”) – useful if you’re taking a day off, or you wake up before your alarm. In the [settings menu](#settings-menu), you can set the alarm to skip automatically during the work week or on weekends – and when this is active, you can also _unskip_ the next alarm by simply switching it back to **on.**
* When the alarm [signals](#signals), press any button – once to snooze, and again to cancel the snooze / silence the alarm for the day (it will give a short low beep, and/or the display will blink once).
  * If the alarm is set to use a [switch signal](#signal), **Alt** will switch it off without snooze.
* **Fibonacci mode** wakes you gradually by starting the alarm about 27 minutes early, by beeping at increasingly shorter intervals per the [Fibonacci sequence](https://en.wikipedia.org/wiki/Fibonacci_number) (610 seconds, then 337, then 233...). This mode is enabled in the [settings menu](#settings-menu), and applies only to [beeper and pulse signals](#signals).
  * Snooze does not take effect in this mode. Any button press will silence the alarm for the day, even if the set alarm time hasn’t been reached yet.

## Chrono/Timer

This feature can count up (chrono) or down (timer), up to 100 hours each way. When idle, it displays `0` (or if you have leading zeros enabled, `000000`).

* To start and stop, press **Up**.
  * While stopped, **Down** will reset to `0`.
* To use the chrono, start from `0`.
  * While the chrono is running, **Down** will display a lap time.
* To use the timer, hold **Select** to set it.
  * When setting, it will prompt for hours/minutes first, then seconds. For convenience, it will recall the last-used time – to reuse this time, simply press **Select** twice. Once the timer is set, press **Up** to start it.
  * While the timer is running, **Down** will cycle through the runout options (what the timer will do when it runs out – clocks with [beeper signal](#signals) only):
    * 1 beep: simply stop, with a long signal (default)
    * 2 beeps: restart, with a short signal (makes a great interval timer!)
    * 3 beeps: start the chrono, with a long signal
    * 4 beeps: start the chrono, with a short signal
* When the timer [signals](#signals), press any button to silence it.
* You can switch displays while the chrono/timer is running, and it will continue to run in the background. It will reset to `0` if you switch displays while it’s stopped, if it’s stopped for an hour, if the chrono reaches 100 hours, or if power is lost.

If your clock uses a knob (rotary encoder) for **Up/Down** rather than buttons, the controls differ slightly:

* **Up** will start; **Down** will stop.
* While the chrono is running, **Up** will display a lap time.
* While the timer is running, **Up** will cycle through the runout options.
* To reset to `0`, switch to another display while stopped. (**Down** does nothing while stopped, to prevent accidental resets.)

## Signals

Your clock can trigger signals for the time of day (chime), alarm, and timer. If your clock supports multiple signal types, you can choose which type is triggered by each function in the [settings menu](#settings-menu).

* **Beeper** signal is played on a piezo beeper, using various patterns and pitches.
* **Switch** signal switches on and off to control an appliance circuit (such as for a radio). If supported, **Alt** acts as a power switch to toggle it on and off manually.
	* If the alarm is set to use this, it will switch on for up to two hours. **Alt** will switch it off without snooze (unlike the other buttons, which will trigger snooze as usual).
	* If the timer is set to use this, it will switch on while the timer is running, like a “sleep” function.
* **Pulse** signal simply sends short pulses (such as to ring a bell).

## Function preset

If your clock supports _neither_ [switch signal](#signals) nor [Wi-Fi](#wi-fi-support), **Alt** acts as a function preset button.

* While viewing the display you want quick access to (such as the alarm or chrono/timer), hold **Alt** until it beeps and the display blinks once; then you can use **Alt** to jump straight there.
  * TIP: If **Alt** is set as the alarm preset, it will switch the alarm as well – so you can check and switch it with a few presses of a single button.

## Settings menu

* To enter the settings menu, hold **Select** for 3 seconds until the hour displays `1`. This indicates setting number 1.
* Use **Up/Down** to go to the setting number you want to set (see table below); press **Select** to open it for setting (display will blink); use **Up/Down** to set; and **Select** to save.
* When all done, hold **Select** to exit the menu.
* If you have a Wi-Fi-enabled clock, you can configure these settings (and more) on the [settings page](#wi-fi-support).

|  | Setting | Options |
| --- | --- | --- |
|  | <a name="settingsgeneral"></a>**General** |  |
| 1 | Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2 | Date format | 1 = month/date/weekday<br/>2 = date/month/weekday<br/>3 = month/date/year<br/>4 = date/month/year<br/>5 = year/month/date<br/>The weekday is displayed as a number from 0 (Sunday) to 6 (Saturday).<br/>Four-digit displays will show only the first two values in each of these options. |
| 3 | Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date each minute at :30 seconds<br/>3 = same as 2, but scrolls in and out |
| 4 | Leading zeros | 0 = no<br/>1 = yes |
| 5 | Digit fade | 0–20, in hundredths of a second<br/>(Clocks with nixie display only) |
| 6 | Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ)<br/>If the clock is not powered at the time, it will correct itself when powered up.<br/>If you observe DST but your locale’s rules are not represented here, leave this set to 0 and set the clock manually (and the [DST offset](#settingsgeography) if applicable). |
| 7 | Backlight | 0 = always off<br/>1 = always on<br/>2 = on until night/away shutoff (if enabled)<br/>3 = on when alarm/timer signals</br>4 = on with [switch signal](#signals) (if equipped)<br/>(Clocks with backlighting only) |
| 8 | Anti-cathode poisoning | Briefly cycles all nixie tubes to prevent [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm)<br/>0 = once a day, either at midnight or when night shutoff starts (if enabled)<br/>1 = at the top of every hour<br/>2 = at the top of every minute<br/>(Will not trigger during night/away shutoff. Clocks with nixie display only) |
|  | <a name="settingsalarm"></a>**Alarm** | (Clocks with signals only) |
| 10 | Alarm auto-skip | 0 = alarm triggers every day<br/>1 = work week only, skipping weekends (per settings below)<br/>2 = weekend only, skipping work week |
| 11 | Alarm signal | 0 = beeper (uses pitch and pattern below)<br/>1 = switch (will stay on for 2 hours)<br/>2 = pulse<br/>(Clocks with multiple signal types only) |
| 12 | Alarm beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 13 | Alarm beeper pattern | 0 = long (1/2-second beep)<br/>1 = short (1/4-second beep)<br/>2 = double (two 1/8-second beeps)<br/>3 = triple (three 1/12-second beeps)<br/>4 = quad (four 1/16-second beeps)<br/>5 = cuckoo (two 1/8-second beeps, descending major third)<br/>(Clocks with beeper only) |
| 14 | Alarm snooze | 0–60 minutes. 0 disables snooze. |
| 15 | [Fibonacci mode](#alarm) | 0 = off<br/>1 = on<br/>(Clocks with [beeper or pulse signals](#signals) only)
|  | <a name="settingstimer"></a>**Chrono/Timer** |  |
| 21 | Timer signal | 0 = beeper (uses pitch and pattern below)<br/>1 = switch (will stay on until timer runs down)<br/>2 = pulse<br/>(Clocks with multiple signal types only) |
| 22 | Timer beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 23 | Timer beeper pattern | Same options as alarm beeper pattern.<br/>(Clocks with beeper only) |
|  | <a name="settingschime"></a>**Chime** |  |
| 30 | Chime | Make noise on the hour:<br/>0 = off<br/>1 = single pulse<br/>2 = [six pips](https://en.wikipedia.org/wiki/Greenwich_Time_Signal) (overrides pitch and pattern settings)<br/>3 = pulse the hour (1 to 12)<br/>4 = ship’s bell (hour and half hour)<br/>Will not sound during night/away shutoff (except when off starts at top of hour)<br/>(Clocks with [beeper or pulse signals](#signals) only) |
| 31 | Chime signal | 0 = beeper (uses pitch and pattern below)<br/>2 = pulse<br/>(Clocks with [beeper and pulse signals](#signals) only) |
| 32 | Chime beeper pitch | [Note number](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8).<br/>(Clocks with beeper only) |
| 33 | Chime beeper pattern | Same options as alarm beeper pattern. Cuckoo recommended!<br/>(Clocks with beeper only) |
|  | <a name="settingsshutoff"></a>**Night/away shutoff** |  |
| 40 | Night shutoff | To save display life and/or preserve your sleep, dim or shut off the display nightly when you’re not around or sleeping.<br/>0 = none (fully on)<br/>1 = dim<br/>2 = shut off<br/>When off, you can press **Select** to illuminate the display briefly. |
| 41 | Night starts at | Time of day. |
| 42 | Night ends at | Time of day. Set to 0:00 to use the alarm time. |
| 43 | Away shutoff | To further save display life, shut off the display during daytime hours when you’re not around. This feature is designed to accommodate your work schedule.<br/>0 = none (on all day every day, except for night shutoff)<br/>1 = clock at work (shut off all day on weekends)<br/>2 = clock at home (shut off during work hours only)<br/>When off, you can press **Select** to illuminate the display briefly. |
| 44 | First day of work week | 0–6 (Sunday–Saturday) |
| 45 | Last day of work week | 0–6 (Sunday–Saturday) |
| 46 | Work starts at | Time of day. |
| 47 | Work ends at | Time of day. |
|  | <a name="settingsgeography"></a>**Geography** |  |
| 50 | Latitude | Your latitude, in tenths of a degree; negative (south) values are indicated with leading zeroes. (Example: Dallas is at 32.8°N, set as `328`.) |
| 51 | Longitude | Your longitude, in tenths of a degree; negative (west) values are indicated with leading zeroes. (Example: Dallas is at 96.7°W, set as `00967`.) |
| 52 | UTC offset | Your time zone’s offset from UTC (non-DST), in hours and minutes; negative (west) values are indicated with leading zeroes. (Example: Dallas is UTC–6, set as `0600`.)<br/>If you observe DST but set the clock manually rather than using the [auto DST feature](#settingsgeneral), you must add an hour to the UTC offset during DST, or the sunrise/sunset times will be an hour early. |

To reset the clock to “factory” settings, hold **Select** for 10 seconds while powering up the clock. You will see the time reset to 0:00.

## Wi-Fi support

If your clock is Wi-Fi-enabled, it offers a settings webpage that duplicates the menu above (and more), and can set itself by synchronizing to an [NTP time server](https://en.wikipedia.org/wiki/Time_server).

**To activate the settings page,** grab a device with a web browser, and briefly hold **Alt**.

* If the clock **is not** connected to Wi-Fi, it will display `7777`.
	* This indicates it is broadcasting a Wi-Fi network called “Clock.” Connect your device to “Clock” and browse to [7.7.7.7](http://7.7.7.7).
	
* If the clock **is** connected to Wi-Fi, it will flash its IP address (as a series of four numbers).
	* Connect your device to the same Wi-Fi network as the clock, and browse to that IP address.
	* If you don’t know what Wi-Fi network the clock is connected to, hold **Alt** for 10 seconds. It will disconnect and broadcast the “Clock” network instead, as above.

When Wi-Fi settings are changed, the clock will immediately attempt to reconnect. As above, it will display its IP address if successful, or `7777` if not. Reconnect to the clock to continue configuring it.

For security, the settings page and “Clock” network (if applicable) will deactivate after two minutes of inactivity.

> If your clock has no **Alt** button, hold **Select** for 5 seconds to activate the page, and 10 seconds to force the “Clock” network.

**When NTP sync is enabled,** the clock will attempt to synchronize every hour (at minute 59). If the clock displays the time without seconds, this indicates the time displayed was not synced to NTP in the last 24 hours. Check to make sure the clock is connected to Wi-Fi and configured to use a valid NTP server, and try a manual sync. If no Wi-Fi is available or you’re unable to sync for other reasons (such as network limitations or manual time setting), disable Wi-Fi or NTP sync to restore the seconds display.