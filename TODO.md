# To-dos

* Timer count up, ending options - maybe separate chrono and timer à la Timex?
* Different setting option for pushbutton (à la Timex) vs. rotary (à la microwave ovens) - external file?
* Option to display weekdays as Sun=0 or Sun=1 (per Portuguese!)
* When setting times of day, make 1439 (minutes) roll over to 0 and vice versa
* Implement options for full date every 5 minutes
* Is it possible to trip the chime *after* determining if we're in night shutoff or not
* Reenable rotary encoder with libraries with workable licenses
* In display code, consider using `delayMicroseconds()` which, with its tighter resolution, may give better control over fades and dim levels
* in `checkInputs()`, can all this if/else business be defined at load instead of evaluated every sample? OR is it compiled that way? maybe use `#ifdef`
* in `ctrlEvt()`, could we do release/shorthold on mainSel so we can exit without making changes?
* Should functions be modular in the code, and have a reserved memory location / 100 per each?
* Should functions have their own options menu?
* I2C display to help with setting?
* I2C multicolor LED to indicate which function we're in?
* Metronome function
* Alarm option should be beeping patterns, including a slow wake which defeats the 2 minute delay
* Signalstart should create a situation where there's time on the counter, but doesn't make sound since the rtc can do that. Other beepable actions would probably cancel that counter anyway

See other TODOs throughout code.