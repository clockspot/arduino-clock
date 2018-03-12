# arduino-nixie
Code for Arduino Nano in [RLB Designs](http://rlb-designs.com/) IN-12/17 clock v5.0

Featuring timekeeping by DS3231 real-time clock and six digits multiplexed 3x2 via two SN74141 driver chips

**This is an alternate codebase that is very much in progress!**

## Instructions

_In these instructions, **Select** is the main pushbutton, and **Adjust** can be a pair of up/down buttons (hold to set faster), or a knob (rotary encoder). Other variations may apply depending on the options selected in the code._

### Clock Functions

* Press **Select** to cycle through the clock's functions.
* Some functions return to Time after a few seconds.
* To set a function, hold **Select** 'til the display flashes; use **Adjust** to set, and **Select** to save.

| Function | Looks like | Notes |
| --- | --- | --- |
| **Time** | `12 34 56` | The time of day. You can choose 12h or 24h format in Options. When setting, it's in 24h format (so you can tell AM from PM) and the seconds will reset to :00 when you save. |
| **Date** | `_2 _4 _0`<br/>(for&nbsp;Sun&nbsp;2/4) | You can choose month/date or date/month format in the options menu (2). Setting is done in three stages: first year, then month, then date.<br/>Weekdays are: 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat |
| **Timer** | `__ _1 23`&nbsp;(running)<br/>`__ __ _0`&nbsp;(stopped) | A countdown timer, in hours, minutes, and seconds. Can be set to the minute, up to 18 hours. Begins running as soon as you set it, and will continue to run in the background if you change to a different function. To cancel a running timer, hold **Select**. When it runs out, press **Select** to silence the beeper. |
| **Day counter** | `_1 23 __` | Shows the number of days until/since a date you specify The target date is set the same way as **Date.** |
| **Thermometer** | `__ 38 25`<br/>(for 38.25°C) | Shows the temperature of the onboard DS3231 chip. May not be very useful as it tends to read higher than ambient temperature and its tolerance is low. |
| **Cleaner** | `88 88 88` | Cycles all the digits on all the tubes. A quick and dirty anti-cathode-poisoning mode, until automatic digit cycling is implemented. |
| **Alarm** | `_7 00 1_` | _Not yet implemented. Intended to work like this:_<br/>Shows alarm time and status. Use **Adjust** to switch on/off; status is shown on 5th tube (1=on, 0=off) and by display brightness (bright=on, dim=off). Hold **Select** to set the time the same way as **Time**. When alarm sounds, press **Select** to snooze, or hold for 1sec (followed by a short beep) to silence the alarm for the day. Options menu lets you restrict the alarm to your workweek or weekend only. |

### Options Menu

* Additional settings are available in the options menu.
* To access this, hold **Select** for 3 seconds until you see a single `1` on the hour tubes. This indicates option number 1.
* Use **Adjust** to go to the option number you want to set (see table below); press **Select** to open it for setting (display will flash); use **Adjust** to set; and **Select** to save.
* When all done, hold **Select** to exit the options menu.

| Option | Settings |
| --- | --- |
| 1. Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2. Date format | 1 = month/date<br/>2 = date/month |
| 3. Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date (as above) every minute at :30 seconds |
| 4. Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |
| 7. Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ) |

**Setup options not implemented yet**

| Option | Settings |
| --- | --- |
| 5. Transition fade | 0–50 (in hundredths of a second) |
| 6. Digit cycle (prevents [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm)) | 0 = before midnight and/or before day-off<br/>1 = every hour before :01 minute |
| 8. Hourly strike | 0 = off<br/>1 = double beep<br/>2 = pips<br/>3 = strike the hour<br/>4 = ship's bell<br/>(Clocks without radio/timer control only. Will not sound during day-off or night-off.) |
| 9. Alarm snooze | 0–60 minutes. 0 disables snooze. |
| 10. Alarm days | 0 = every day<br/>1 = workdays only (per options 15/16)<br/>2 = non-workdays only |
| 11. Night-off | To save tube life or preserve your sleep, dim or shut off tubes when you're not around or sleeping.<br/>0 = none (tubes fully on at night)<br/>1 = dim tubes at night<br/>2 = shut off tubes at night<br/>When off, you can press **Select** to light the tubes briefly. |
| 12. Night starts at | Time of day |
| 13. Night ends at | Time of day. Set to 0:00 to use the alarm time. |
| 14. Day-off | To save tube life, shut off tubes during the day when you're not around.<br/>0 = none (tubes fully on during day)<br/>1 = clock at work (shut off all day on weekends)<br/>2 = clock at home (shut off on workdays during work hours)<br/>When off, you can press **Select** to light the tubes briefly. |
| 15. First day of work week | 0–6 (Sunday–Saturday) |
| 16. Last day of work week | 0–6 (Sunday–Saturday) |
| 17. Work starts at | Time of day |
| 18. Work ends at | Time of day |
| 19. Fine regulation | Adjusts clock's timekeeping in tenths of a second per week. 500 is "normal." (e.g.: 503 causes clock to run 0.3s faster per week.) |