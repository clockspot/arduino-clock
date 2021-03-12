# To-dos

* Rename project to arduino-clock with first push of v2+ - replace instances of arduino-nixie within
* Reintroduce nixie clean and scroll
* Persistent storage on EEPROM - find a solution that writes to flash less often
* Network/NTP
	* Persistent storage for wi-fi credentials!
	* Why does the page sometimes drop?
	* wi-fi credential save fails if keys are part of the string?
	* DST calc may behave unpredictably between 1–2am on fallback day
	* Redo NTP on startup if it failed (networkStartWifi())
	* Handle 2038+ epochs
	* Notice when a leap second is coming and handle it
	* When setting page is used to set day counter and date, and the month changes, set date max. For 2/29 it should just do 3/1 probably.
  * Weather support
* Input: support other IMU orientations
* When day counter is set to count up from 12/31, override to display 365/366 on that date
* Bitmask to enable/disable features?
* Option to display weekdays as Sun=0 or Sun=1 (per Portuguese!)
* Is it possible to trip the chime *after* determining if we're in night shutoff or not
* In display code, consider using `delayMicroseconds()` which, with its tighter resolution, may give better control over fades and dim levels
* in `ctrlEvt()`, could we do release/shorthold on mainSel so we can exit without making changes?
* I2C multicolor LED to indicate which function we're in? - possibly as part of display
* Metronome function?
* Signalstart should create a situation where there's time on the counter, but doesn't make sound since the rtc can do that. Other beepable actions would probably cancel that counter anyway (is this still applicable?)
* Why does the display flicker sometimes? are we doubling up on a display cycle call?

See other TODOs throughout code.