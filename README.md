# arduino-nixie
Code for Arduino Nano in [RLB Designs](http://rlb-designs.com/) IN-12/17 clock v5.0

Featuring timekeeping by DS3231 real-time clock and six digits multiplexed 3x2 via two SN74141 driver chips

**This is an alternate codebase that is very much in progress!**

## Instructions

_In these instructions, **Select** is the main pushbutton, and **Adjust** can be a pair of up/down buttons (hold to set faster), or a knob (rotary encoder). Other variations may apply depending on the options selected in the code._

Press **Select** to cycle through the clock's functions. Some functions return to Time after a few seconds.

To set a function, hold **Select** 'til the display flashes; use **Adjust** to set, and **Select** to save.

| Function | Notes |
| --- | --- |
| **Time** shows the time of day, e.g. `12 34 56` for 12:34:56. | You can choose 12h or 24h format in Options. When setting, it's in 24h format (so you can tell AM from PM) and the seconds will reset to :00 when you save. |
| **Date** shows the month, date, and weekday. e.g. `_2 _4 _0` for Sunday, February 4th. | The weekday is 0=Sunday, 6=Saturday. You can choose month/date or date/month format in Options. Setting is done in three stages: first year, then month, then date. |
| **Day counter** shows the number of days until/since a date you specify, e.g. `_1 23 __` for 123 days. | The target date is set the same way as **Date.** |
| **Thermometer** shows the temperature of the onboard DS3231 chip, e.g. `__ 38 25` for 38.25°C (I think). | This may not be very useful as it tends to read higher than ambient temperature and its tolerance is low. |
| **Alarm** | _Not yet implemented_ |
| **Timer** | _Not yet implemented_ |

Additional settings are available in the options menu. To access this, hold **Select** for 3 seconds until the display flashes something like `__ _0 _1`. The right tubes indicate you are viewing option 1, and the left tubes indicate its current setting. Use **Select** to cycle through options; **Adjust** to change the setting; and hold **Select** to exit.

| Option | Settings |
| --- | --- |
| 1. Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2. Date display | 1 = month/date/weekday<br/>2 = date/month/weekday |
| 3. Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date (as above) every minute at :30 seconds *(not implemented)* |
| 4. Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |

**Setup options not implemented yet**

| Option | Settings |
| --- | --- |
| 5. Transition fade | 0–50 (in hundredths of a second) |
| 6. Digit cycle (prevents [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm)) | 0 = before midnight and/or before day-off<br/>1 = every hour before :01 minute |
| 7. Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ) |
| 8. Hourly strike | 0 = off<br/>1 = double beep<br/>2 = pips<br/>3 = strike the hour<br/>4 = ship's bell<br/>(Clocks without radio/timer control only. Will not sound during day-off or night-off.) |
| 9. Snooze duration | 0–60 minutes. 0 disables snooze. |
| 10. Fine regulation | Adjusts clock's timekeeping in tenths of a second per week. 500 is "normal." (e.g.: 503 causes clock to run 0.3s faster per week.) |
| 11. Night-off | 0 = normal (tubes fully on at night)<br/>1 = dim tubes at night<br/>2 = shut off tubes at night |
| 12. Night starts at | Time of day |
| 13. Night ends at | Time of day. Set to 0:00 to use the alarm time. |
| 14. Day-off | To save tube life, shut off tubes during the day when you're not around.<br/>0 = normal (no day-off)<br/>1 = clock at work (shut off all day on weekends)<br/>2 = clock at home (shut off during work hours) |
| 15. First day of work week | 0–6 (Sunday–Saturday) |
| 16. Last day of work week | 0–6 (Sunday–Saturday) |
| 17. Work starts at | Time of day |
| 18. Work ends at | Time of day |