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
	* Functionize raw variable access in ctrlEvt (in progress)
* Other TODOs in esp32-proton.h

Prototype text:
This project replaces the clock module, with its TI IC brain, in a Proton 320 clock radio. (The radio section is left alone.) It works much the same as the original, with these enhancements:

* **Larger, crisper display.** The eight-digit VFD is replaced with a six-digit LCD. Ambient lighting still controls its brightness. Alarm time is displayed when the alarm is switched on.

* **Easier setting.** To set anything, simply hold Snooze for 1sec to enter setting mode; use the Up/Down buttons to adjust, and Snooze again to save. No need to toggle the set switch in back.

* **NTP sync via WiFi.** When this is enabled, a normal seconds display indicates the time was synced in the last hour; otherwise the seconds will blink to indicate low confidence in the displayed time.

* **Perpetual calendar.** Even without NTP sync, the calendar now accounts for leap years. When setting, the year, month, and date are adjusted separately; Snooze cycles between them.

* **Date/sun/weather display.** Pressing Snooze (except during alarm or setting) will display the date, followed by sunrise/sunset and weather information, if enabled in config (below).

* **Web-based config menu.** This menu allows configuration of display formats, WiFi settings, automatic DST changes, alarm schedules, and more. It is accessed by [TBD] which causes the radio to broadcast a WiFi network called "Proton 320"; then connecting any web device to this network and browsing to 7.7.7.7.


## Tube-driving fix

The nixie display code performs multiplexing, where only two of the six (or four) digits are lit at once. It is possible for one or both of these digits to be blanked, because no cathode (via SN74141) is connected. I have noticed that some of my clocks, which have been in use the most, develop a ghosting problem where - if all the tubes (or maybe just all the tubes in a given multiplex set) are lit, it's OK, but if some are not, all the filaments in all the tubes in the set have a soft glow, like all the cathodes are connected.

At first I thought this was a display/tube fault, but I swapped displays and the fault went with the driver board (UNDB). So then I thought it was a fault in the SN74141 chips, but replacing them didn't help. So now I think it might be a fault in the Arduino itself. And I suspect it's related to too much current going through something, or going where it shouldn't, when a tube is not lit. I've tried to stave off the fault in my still-working clocks by avoiding blank tubes (turning on leading zeroes, etc).

**AI sez:**
Per the schematic and the multiplex code:

**What's happening on the board.** Each anode channel is an Arduino pin driving a small-signal transistor that level-shifts to a high-voltage switch sending ~170V to two tubes simultaneously (1/4, 2/5, 3/6). The cathodes return through the SN74141, which decodes a 4-bit BCD input --- values 10--15 produce no active cathode, which is how the code blanks. In a healthy circuit, sending 15 means no current path → tube dark. On a degraded board, some weak leakage path completes the circuit anyway during slices where the cathode chip is in "off" decode, so multiple cathodes glow faintly. Most likely culprit, given that the fault travels with the UNDB and SN74141 swap didn't help: aged Arduino GPIO drivers with elevated leakage on the SN74141 input lines (partially activating cathodes) or anode-driver leakage. Sustained years of use can degrade GPIO output stages well past datasheet typicals.

Your existing user-side mitigation (leading zeros on, no blanks in normal display) is the right answer for that case.

[We did update the code so that when **both** tubes in an anode pair would be blank, we skip firing the anode entirely for that slice.]

**What's the actual failure mode?** Two likely culprits, both consistent with progressive degradation on heavily-used clocks:

1.  **Arduino GPIO output stages aging** from years of toggling at multiplex frequency (~600 Hz × 8 cathode pins + 3 anode pins). MOSFET output drivers can develop elevated leakage in their off state --- datasheet "low" current rises from nA into µA range, enough to weakly bias the SN74141 inputs into partial-decode states.
2.  **Anode-driver transistor leakage** on the UNDB itself --- the level-shift transistor's off-state collector current creeps up over years of HV switching, slightly biasing the HV anode rail even when "off."

The software fix above eliminates the worst case (no tube should be lit) but can't help when one tube in a pair is lit and the other is blank, because the anode has to fire.

**Hardware mitigations, cheapest to most invasive:**

| Approach | Effort | Effect |
| --- |  --- |  --- |
| **74HC244 / 74HC4050 buffer between Arduino and SN74141** | One IC, ~$0.30, drops onto a future board rev. Each Arduino pin drives a buffer input; the buffer's clean rail-to-rail output drives the SN74141. | Absorbs Arduino GPIO leakage; the buffer chip stays at logic 0 even if the Arduino pin leaks µA. Cheapest insurance. Doesn't fix anode-driver leakage. |
| --- |  --- |  --- |
| **74HC595 shift register driving the SN74141 inputs** | One IC, SPI-style update. Reduces Arduino → 3 pins (clock/data/latch) toggling at SPI rate instead of 8 pins at multiplex rate. | Same buffer benefit as above, plus an order of magnitude less wear on the Arduino's GPIO stages going forward. Slight code rewrite (`setCathodes` becomes a serial shift instead of bit-banged digitalWrites). |
| **HV5122 / HV5222 high-voltage shift registers** | Replaces both SN74141s AND the discrete anode driver transistors. SPI in, HV out, 32 channels per chip --- enough to drive every cathode of every tube directly without multiplexing at all. Used in modern nixie clock designs. | Eliminates the multiplex entirely → no anode-pair ghosting possible, brighter tubes (100% duty cycle), simpler PCB. Bigger redesign --- UNDB v10 territory. |
| **Optoisolation between Arduino and HV side** | 6N137 or similar between Arduino and anode driver. Galvanic separation. | Bulletproof against Arduino-side faults reaching HV. Adds parts cost, doesn't help cathode side, and somewhat overkill given the cheaper buffer fixes. |

**My recommendation for the next UNDB rev:** drop in a 74HC595 for the cathode lines and a 74HC244 (or another '595) for the anodes. Two cheap chips, one minor firmware rewrite (`setCathodes` becomes a 16-bit SPI shift), and the entire "Arduino GPIO degradation causes ghosting" failure mode goes away. If you're willing to do a deeper redesign, the HV5122/5222 path is the modern answer --- multiplexing-free direct-drive nixie boards have largely supplanted SN74141-based designs in current hobbyist projects for exactly the longevity reasons you're seeing.