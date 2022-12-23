# To-dos

* Startup SEL hold doesn't seem to work with IMU
* Reintroduce nixie clean and scroll
* Persistent storage on EEPROM - find a solution that writes to flash less often
* Network/NTP
	* Why does the page sometimes drop?
	* wi-fi credential save fails if keys are part of the string?
	* DST calc may behave unpredictably between 1–2am on fallback day
	* Redo NTP if it failed (networkStartWifi()) - per bad response - how to make it wait a retry period
	* Make [2036-ready](https://en.wikipedia.org/wiki/Year_2038_problem#Network_Time_Protocol_timestamps)
	* Notice when a leap second is coming and handle it
	* When setting page is used to set day counter and date, and the month changes, set date max. For 2/29 it should just do 3/1 probably.
  * Weather support
  * Stop using strings? There is plenty of RAM available on SAMD I think
* When entering web mode, exit setting mode
* Disable time setting if sync is enabled? Or disable sync if time is set?
* Input: support other IMU orientations
* When day counter is set to count up from 12/31, override to display 365/366 on that date
* Bitmask to enable/disable features?
* Option to display weekdays as Sun=0 or Sun=1 (per Portuguese!)
* Is it possible to trip the chime *after* determining if we're in off-hours or not
* In display code, consider using `delayMicroseconds()` which, with its tighter resolution, may give better control over fades and dim levels
* in `ctrlEvt()`, could we do release/shorthold on mainSel so we can exit without making changes?
* I2C multicolor LED to indicate which function we're in? - possibly as part of display
* Metronome function?
* Signalstart should create a situation where there's time on the counter, but doesn't make sound since the rtc can do that. Other beepable actions would probably cancel that counter anyway (is this still applicable?)
* Why does the display flicker sometimes? are we doubling up on a display cycle call? - we may have fixed this, test
* When using ambient lighting control, alarm on state doesn't appear at full brightness because display code doesn't know *that* bright=2 is deliberate

See other TODOs throughout code.

## Proton branch items

* Port to ESP32 - may make input/network/rtc/storage redundant
* Break input into inputSimple and inputProton; move ctrlEvt into these (arduino-clock should become ctrl-agnostic)

Prototype text:
This project replaces the clock module, with its TI IC brain, in a Proton 320 clock radio. (The radio section is left alone.) It works much the same as the original, with these enhancements:

* **Larger, crisper display.** The eight-digit VFD is replaced with a six-digit LCD. Ambient lighting still controls its brightness.

* **NTP sync via WiFi.** When this is enabled, a normal seconds display indicates the time was synced in the last hour; otherwise the seconds will blink to indicate low confidence in the displayed time.

* **Perpetual calendar.** Even without NTP sync, the calendar now accounts for leap years. When setting, the year, month, and date are adjusted separately; Snooze cycles between them.

* **Date/sun/weather display.** Pressing Snooze (except during alarm or setting) will display the date, followed by sunrise/sunset and weather information, if enabled in config (below).

* **Easier alarm setting.** As there is no longer a dedicated display for the alarm time, it is displayed when the alarm is switched on, and can be adjusted at that time. There's no longer any need to set the rear switch to "Alarm", as this has been co-opted for —

* **Web-based config menu.** This menu allows configuration of display formats, WiFi settings, automatic DST changes, alarm schedules, and more. It is accessed by setting the rear switch to "Alarm" and pressing On, which causes the radio to broadcast a WiFi network called "Proton 320"; then connecting any web device to this network and browsing to 7.7.7.7.