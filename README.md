# arduino-nixie
Code for Arduino Nano in RLB Designs IN-12/17 clock v5.0
featuring timekeeping by DS1307 RTC and six digits multiplexed 3x2 via two SN74141 driver chips

This is an alternate codebase that is very much in progress!

## Current instructions (WIP)

* Normal running mode simply shows a number (42 at startup).
  * To set, hold S4 'til display flashes; push/hold S2/S3 to set the number up/down; push S4 to save.

* To enter the *Setup* menu, hold for 3 seconds.
  * Shows option number on small tubes, and current setting on big tubes.
  * Push to cycle through options (listed below); turn to set; hold to exit.

| Option | Possible Settings |
| --- | --- |
| 1. Dimming | 0 = Normal running mode will display at full brightness<br/>1 = Normal running mode will display dim (25% duty cycle) |
| 2. Nothing | Any number from 1 to 7. Has no effect yet. |
| 3. Nothing | Any number from 1 to 13. Has no effect yet. |

## Todos

* Add hold-advance for adj buttons
* Add support for rotary encoders
  * Add velocity
  * Configurable run controls
  * Test
* Flesh out menu
  * Make settings save in eeprom(?)
* Reintroduce Time
  * Make settable - if setting is opened in latter half of minute, setting value should be min+1 to make sync easy
* Date
  * Make settable
* Alarm
  * Make settable
  * Make trigger/switch, snooze, and silence
* Timer
  * Make settable (opens to last setting)
  * Make run, trigger/switch, silence
* Auto display reset
  * When on a mode other than clock or running timer, reset to clock after 5 sec
  * When in setting mode or setup menu, exit without saving after 120 seconds
* "Maybe later" features
  * Thermometer?
  * Countdown to date?
  * Sunrise/sunset calc per lat/long?

## Proposed instructions

_These instructions are for a clock equipped with a single knob/button (rotary encoder). Instructions will vary with the controls equipped and options selected in code. If a hardware alarm switch is equipped, **Alarm** may have no software switch._

**Turn the knob to see each of the clock's functions:**

* **Time**
  * Shows the current time of day. (Choose 12h or 24h format in Setup.)
  * To set, hold 'til display flashes; turn to set (in 24h format), and push to save at top of minute (seconds will set to :00).
* **Date**
  * Shows month, date, and weekday as 0=Sunday, 6=Saturday. (Choose month/date or date/month in Setup.)
  * To set, hold 'til display flashes; then set year, then month, then date.
* **Alarm**
  * Shows alarm time (in 24h format) and whether the alarm is on (1) or off (0).
  * To turn alarm on or off, push the knob.
  * To set alarm, hold 'til display flashes; turn to set (in 24h format), and push to save.
  * When alarm goes off, push to snooze, or hold to silence until tomorrow.
* **Timer**
  * Shows a countdown timer (or 0min 00sec when idle).
  * To set, hold 'til display flashes; set time (in hrs, min, sec); and press to start. Hold to cancel.
  * When timer goes off, push to silence.

**Additional settings are available in the Setup menu.**

* To enter **Setup**, hold for 3 seconds.
  * Shows option number on small tubes, and current setting on big tubes.
  * Push to cycle through options (listed below); turn to set; hold to exit.

| Option | Possible Settings |
| --- | --- |
| 1. Time format | 12- or 24-hour (time display only; setting is always done in 24h) |
| 2. Date display | 1 = month/date/weekday<br/>2 = date/month/weekday |
| 3. Display date during time? | 0 = never<br/>1 = date instead of seconds<br/>2 = full date (as above) each minute at :30 seconds |
| 4. Leading zero in hour, date, and month? | 0 = no<br/>1 = yes |
| 5. Transition fade | 0–50 (in hundredths of a second) |
| 6. Digit cycle (prevents [cathode poisoning](http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm) | 0 = before midnight and/or before day-off<br/>1 = each hour at :01 minute |
| 7. Auto DST | Add 1h for daylight saving time between these dates (at 2am):<br/>0 = off<br/>1 = second Sunday in March to first Sunday in November (US/CA)<br/>2 = last Sunday in March to last Sunday in October (UK/EU)<br/>3 = first Sunday in April to last Sunday in October (MX)<br/>4 = last Sunday in September to first Sunday in April (NZ)<br/>5 = first Sunday in October to first Sunday in April (AU)<br/>6 = third Sunday in October to third Sunday in February (BZ) |
| 8. Hourly strike | 0 = off<br/>1 = beep<br/>2 = pips<br/>3 = strike<br/>4 = ship's bell<br/>(Clocks without radio/timer control only. Will not sound during day-off or night-off.) |
| 9. Snooze duration | 0–60 minutes. 0 disables snooze. |
| 10. Fine regulation | Adjusts clock's timekeeping in tenths of a second per week. 500 is "normal." (e.g.: 503 causes clock to run 0.3s faster per week.) |
| | *Night-off mode* – Dim or shut off the tubes daily. |
| 11. Night-off mode | 0 = normal (no change)<br/>1 = dim tubes at night<br/>2 = shut off tubes at night |
| 12. Night starts at | Time of day |
| 13. Night ends at | Time of day. Set to 99:99 to use the alarm time. |
| 14. Day-off mode | To save tube life, shut off tubes during the day when you're not around.<br/>0 = normal (no change)<br/>1 = office mode (shut off all day on weekends)<br/>2 = home mode (shut off during office hours) |
| 15. First day of work week | 0–6 (Sunday–Saturday) |
| 16. Last day of work week | 0–6 (Sunday–Saturday) |
| 17. Work starts at | Time of day |
| 18. Work ends at | Time of day |