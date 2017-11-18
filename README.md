# arduino-nixie
Code for Arduino Nano in [RLB Designs](http://rlb-designs.com/) IN-12/17 clock v5.0

Featuring timekeeping by DS1307 real-time clock and six digits multiplexed 3x2 via two SN74141 driver chips

**This is an alternate codebase that is very much in progress!**

## Instructions

_In these instructions, "Select" is the main pushbutton, and "Adjust" can be either a knob (rotary encoder) or a pair of up/down buttons (hold to set faster). Other variations may apply depending on the options selected in the code._

**Use Adjust to see each of the clock's functions:** (~crossed out~ = not implemented)

* **Time**
  * Shows the current time of day. (Choose 12h or 24h format in Setup.)
  * To set time, hold Select 'til display flashes; use Adjust to set (in 24h format), and push Select to save at the top of minute (seconds will set to :00).
* **Date**
  * Shows month, date, and weekday as 0=Sunday, 6=Saturday. (Choose month/date or date/month in Setup.)
  * To set date, hold Select 'til display flashes; then set year, then month, then date, pushing Select to save each.
* ~**Alarm**~
  * ~Shows alarm time (in 24h format) and whether the alarm is on (1) or off (0).~
  * ~To turn alarm on or off, press Select.~
  * ~To set alarm, hold Select 'til display flashes; use Adjust to set (in 24h format), and push Select to save.~
  * ~When alarm goes off, push Select to snooze, or hold to silence until tomorrow.~
* ~**Timer**~
  * ~Shows a countdown timer (or 0min 00sec when idle).~
  * ~To set timer, hold Select 'til display flashes; set time (in hrs, min, sec); and push Select to start. Hold to cancel.~
  * ~When timer goes off, push Select to silence.~

**Additional settings are available in the Setup menu.**

* To enter **Setup**, hold Select for 3 seconds, until "1" appears on the small tubes, which indicates you are viewing the first option (see below). Its current setting is shown on the big tubes.
  * Push Select to cycle through options; use Adjust to set; hold Select to exit.

| Option | Possible Settings |
| --- | --- |
| 1. Time format | 1 = 12-hour<br/>2 = 24-hour<br/>(time-of-day display only; setting times is always done in 24h) |
| 2. Date display | 1 = month/date/weekday<br/>2 = date/month/weekday |
| 3. Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>~2 = full date (as above) every minute at :30 seconds~ |
| 4. Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |

**Setup options not implemented yet**

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