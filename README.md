# arduino-nixie
**A digital clock with perpetual calendar, alarm, countdown timer, and day counter.** Written for the Arduino Nano at the heart of [RLB Designs'](http://rlb-designs.com/) Universal Nixie Driver Board (UNDB) v5.0, featuring a DS3231 thermocompensated battery-backed real-time clock, and driving up to 6 digits multiplexed in pairs via two SN74141 driver chips. Uses AdaEncoder and ooPinChangeInt (for rotary encoders, optional) and NorthernWidget DS3231 libraries.

## Instructions

_In these instructions, **Select** is the main pushbutton, and **Adjust** can be a pair of up/down buttons (hold to set faster), or a knob (rotary encoder). Other variations may apply depending on the options selected in the code, and functions may be reordered or left out._

### Clock Functions

* Press **Select** to cycle through the clock's functions. Some return to Time after a few seconds.
* To set a function, hold **Select** 'til the display flashes; use **Adjust** to set, and **Select** to save.

| Function | Looks like | Notes |
| --- | --- | --- |
| **Time** | `12 34 56` | The time of day. You can choose 12h or 24h format in Options. When setting, it's in 24h format (so you can tell AM from PM) and the seconds will reset to :00 when you save. The clock keeps time during power outages and compensates for temperature effects. |
| **Date** | `_2 _4 _0`<br/>(for&nbsp;Sun&nbsp;2/4) | You can choose the date format in the options menu (2). Setting is done in three stages: first year, then month, then date.<br/>Weekdays are: 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat |
| **Alarm** | `_7 00 1_` | Shows alarm time (always in 24hr format) and on/off status on 5th tube (1=on, 0=off) and by display brightness (bright=on, dim=off). Use **Adjust** to switch on/off. Hold **Select** to set time (same way as **Time**). When alarm sounds, press **Select** to snooze, or hold for 1sec (followed by a short beep) to silence the alarm for the day. Options menu lets you restrict the alarm to your workweek or weekend only. In a power outage, the alarm will remain set, but it will not sound if power is disconnected at alarm time. |
| **Timer** | `__ __ _0` | A countdown timer, in hours, minutes, and seconds; or `0` when stopped. Can be set to the minute, up to 18 hours. Begins running as soon as you set it, and will continue to run in the background if you change to a different function. To cancel while running, hold **Select**. When timer runs out, press **Select** to silence. If power is lost, the timer will reset to `0`. |
| **Day counter** | `_1 23 __` | Shows the number of days until/since a date you specify. Set the same way as **Date.** |
| **Thermometer** | `__ 38 25` | Shows the temperature of the onboard DS3231 chip (e.g. 38.25°C – I think). May not be very useful as it tends to read higher than ambient temperature and its tolerance is low. |
| **Tube tester** | `88 88 88` | Cycles through all the digits on all the tubes. |

### Options Menu

* To access this, hold **Select** for 3 seconds until you see a single `1` on the hour tubes. This indicates option number 1.
* Use **Adjust** to go to the option number you want to set (see table below); press **Select** to open it for setting (display will flash); use **Adjust** to set; and **Select** to save.
* When all done, hold **Select** to exit the options menu.

| Option | Settings |
| --- | --- |
| **Timekeeping and display** ||
| 1. Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2. Date format | 1 = month/date/weekday<br/>2 = date/month/weekday<br/>3 = month/date/year<br/>4 = date/month/year<br/>5 = year/month/date<br/>Note that four-tube clocks will not display the third value. |
| 3. Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date (as above) every minute at :30 seconds |
| 4. Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |
| 5. Digit fade | _Not yet implemented._<br/>0–50 (in hundredths of a second) |
| 6. Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ) |
| **Alarms and sounds** ||
| 7. Alarm days | 0 = every day<br/>1 = work week only (per settings below)<br/>2 = weekend only |
| 8. Alarm snooze | 0–60 minutes. 0 disables snooze. |
| 9. Alarm signal pitch | [Note number on a piano keyboard](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8). Some are louder than others! |
| 10. Timer interval mode | What happens when the timer reaches 0.<br/>0 = stop and sound continuously<br/>1 = restart and sound a single tone (interval timer) |
| 11. Timer signal pitch | Set the same way as the alarm pitch, above. |
| 12. Hourly strike | 0 = off<br/>1 = single beep<br/>2 = pips<br/>3 = strike the hour<br/>4 = ship's bell<br/>Clocks without radio/timer control only. Will not sound during day-off/night-off (except when entering day-off/night-off at the top of the hour). |
| 13. Hourly strike pitch | Set the same way as the alarm signal pitch, above. If using the pips, 63 (987 Hz) is closest to the real BBC pips frequency (1000 Hz). |
| **Night-off and day-off** ||
| 14. Night-off | To save tube life and/or preserve your sleep, dim or shut off tubes nightly when you're not around or sleeping.<br/>0 = none (tubes fully on at night)<br/>1 = dim tubes at night<br/>2 = shut off tubes at night<br/>When off, you can press **Select** to illuminate the tubes briefly. |
| 15. Night starts at | Time of day. |
| 16. Night ends at | Time of day. Set to 0:00 to use the alarm time. At this time (whether night-off/alarm is enabled or not), all tubes will briefly cycle through all digits at full brightness to help prevent [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm). |
| 17. Day-off | To further save tube life, shut off tubes during the day when you're not around.<br/>0 = none (tubes fully on during the day)<br/>1 = clock at work (shut off all day on weekends)<br/>2 = clock at home (shut off during work hours)<br/>When off, you can press **Select** to illuminuate the tubes briefly. |
| 18. First day of work week | 0–6 (Sunday–Saturday) |
| 19. Last day of work week | 0–6 (Sunday–Saturday) |
| 20. Work starts at | Time of day. |
| 21. Work ends at | Time of day. |

### Factory Reset

To reset the clock to "factory" defaults, hold **Select** while connecting the clock to power.