# arduino-nixie
Code for Arduino Nano in RLB Designs IN-12/17 clock v5.0
featuring timekeeping by DS1307 RTC and six digits multiplexed 3x2 via two SN74141 driver chips

This is an alternate codebase that is very much in progress!

## Current feature set
* Normal running mode
  * Display shows a simple 4-digit counter, which increments when you press S5.
* SETUP menu
  * To enter the SETUP menu, hold S5 for 5 seconds. This is a numbered menu; the small tubes show the current menu item (listed below), and the big tubes show its selected setting.
  * Press S5 to advance through the items; press S2/S3 to change the setting up and down.
  * Hold S5 to exit.

### SETUP menu items
| No. | Name | Possible Settings |
| --- | --- | --- |
| 1 | Dimming | 0 = Normal running mode (the counter) will display at full brightness<br/>1 = Normal running mode will display dim (25% duty cycle) |
| 2 | Nothing | Any number from 1 to 7. Has no effect yet. |
| 3 | Nothing | Any number from 1 to 13. Has no effect yet. |