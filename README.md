# arduino-nixie
**A digital clock with perpetual calendar, alarm, countdown timer, and day counter.** Written for the Arduino Nano at the heart of [RLB Designs'](http://rlb-designs.com/) Universal Nixie Driver Board (UNDB) v5.0, featuring a DS3231 thermocompensated battery-backed real-time clock, and driving up to 6 digits multiplexed in pairs via two SN74141 driver chips. Uses AdaEncoder and ooPinChangeInt (for rotary encoders, optional) and NorthernWidget DS3231 libraries.

**This is an alternate codebase that is very much in progress!**

## Instructions

_In these instructions, **Select** is the main pushbutton, and **Adjust** can be a pair of up/down buttons (hold to set faster), or a knob (rotary encoder). Other variations may apply depending on the options selected in the code._

### Clock Functions

* Press **Select** to cycle through the clock's functions. Some return to Time after a few seconds.
* To set a function, hold **Select** 'til the display flashes; use **Adjust** to set, and **Select** to save.

| Function | Looks like | Notes |
| --- | --- | --- |
| **Time** | `12 34 56` | The time of day. You can choose 12h or 24h format in Options. When setting, it's in 24h format (so you can tell AM from PM) and the seconds will reset to :00 when you save. The clock keeps time during power outages, and its timekeeping is corrected for temperature changes. |
| **Date** | `_2 _4 _0`<br/>(for&nbsp;Sun&nbsp;2/4) | You can choose month/date or date/month format in the options menu (2). Setting is done in three stages: first year, then month, then date.<br/>Weekdays are: 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat |
| **Timer** | `__ __ _0` | A countdown timer, in hours, minutes, and seconds; or `0` when stopped. Can be set to the minute, up to 18 hours. Begins running as soon as you set it, and will continue to run in the background if you change to a different function. To cancel while running, hold **Select**. When timer runs out, press **Select** to silence the beeper. If power is lost, the timer will reset to `0`. |
| **Day counter** | `_1 23 __` | Shows the number of days until/since a date you specify. The target date is set the same way as **Date.** |
| **Thermometer** | `__ 38 25` | Shows the temperature of the onboard DS3231 chip (e.g. 38.25°C – I think). May not be very useful as it tends to read higher than ambient temperature and its tolerance is low. |
| **Cleaner** | `88 88 88` | Cycles all the digits on all the tubes. A quick and dirty anti-cathode-poisoning mode, until automatic digit cycling is implemented. |
| **Alarm** | `_7 00 1_` | _Not yet implemented._<br/>Shows alarm time and status. Use **Adjust** to switch on/off; status is shown on 5th tube (1=on, 0=off) and by display brightness (bright=on, dim=off). Hold **Select** to set the time the same way as **Time**. When alarm sounds, press **Select** to snooze, or hold for 1sec (followed by a short beep) to silence the alarm for the day. Options menu lets you restrict the alarm to your workweek or weekend only. In a power outage, the alarm will remain set, but it will not sound if power is disconnected at alarm time. |

### Options Menu

* Additional settings are available in the options menu.
* To access this, hold **Select** for 3 seconds until you see a single `1` on the hour tubes. This indicates option number 1.
* Use **Adjust** to go to the option number you want to set (see table below); press **Select** to open it for setting (display will flash); use **Adjust** to set; and **Select** to save.
* When all done, hold **Select** to exit the options menu.

| Option | Settings |
| --- | --- |
| **Timekeeping and display** | |
| 1. Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2. Date format | 1 = month/date<br/>2 = date/month |
| 3. Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date (as above) every minute at :30 seconds |
| 4. Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |
| 5. Digit fade | _Not yet implemented._<br/>0–50 (in hundredths of a second) |
| 6. Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ) |
| **Alarms and sounds** | |
| 7. Alarm days | 0 = every day<br/>1 = workweek only (per workdays setting below)<br/>2 = weekend only |
| 8. Alarm snooze | 0–60 minutes. 0 disables snooze. |
| 9. Alarm tone pitch | _Not yet implemented._<br/>[Note number on a piano keyboard](https://en.wikipedia.org/wiki/Piano_key_frequencies), from 49 (A4) to 88 (C8). Some are louder than others. |
| 10. Timer interval mode | _Not yet implemented._<br/>What happens when the timer reaches 0.<br/>0 = stop and sound continuously<br/>1 = restart and sound a single tone (interval timer) |
| 11. Timer tone pitch | _Not yet implemented._<br/>Set the same way as the alarm tone pitch, above. |
| 12. Hourly strike | _Not yet implemented._<br/>0 = off<br/>1 = double tone<br/>2 = pips<br/>3 = strike the hour<br/>4 = ship's bell<br/>(Clocks without radio/timer control only. Will not sound during day-off or night-off.) |
| 13. Hourly strike pitch | _Not yet implemented._<br/>Set the same way as the alarm tone pitch, above. |
| **Night-off and day-off** | |
| 14. Night-off | _Not yet implemented._<br/>To save tube life and/or preserve your sleep, dim or shut off tubes nightly when you're not around or sleeping.<br/>0 = none (tubes fully on at night)<br/>1 = dim tubes at night<br/>2 = shut off tubes at night<br/>When off, you can press **Select** to light the tubes briefly. |
| 15. Night starts at | _Not yet implemented._<br/>Time of day. |
| 16. Night ends at | _Not yet implemented._<br/>Time of day. Set to 0:00 to use the alarm time. At this time (whether night-off/alarm is enabled or not), all tubes will briefly cycle through all digits at full brightness to help prevent [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm). |
| 17. Day-off | _Not yet implemented._<br/>To further save tube life, shut off tubes during the day when you're not around.<br/>0 = none (tubes fully on during the day)<br/>1 = clock at work (shut off all day on weekends)<br/>2 = clock at home (shut off during work hours)<br/>When off, you can press **Select** to light the tubes briefly. |
| 18. First day of work week | 0–6 (Sunday–Saturday) |
| 19. Last day of work week | 0–6 (Sunday–Saturday) |
| 20. Work starts at | _Not yet implemented._<br/>Time of day. |
| 21. Work ends at | _Not yet implemented._<br/>Time of day. |

### Factory Reset

To reset the clock to "factory" defaults, hold **Select** while connecting the clock to power.