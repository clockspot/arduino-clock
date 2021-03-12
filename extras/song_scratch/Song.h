//Try to make the beeper play Woody's chorus part from "1612"
class Song {
  int bpm = 80;
  int pin = 10; //that piezo is connected to
  int dur = 10000; //max note length in millis
  //This'll be like a proto-MIDI with events that represent sounds (notes, chords) or to stop sound.
  //Need to represent chords as a single event since the beeper is mono and has to cycle through the notes.
  //Event timing is in centibeats (e.g. in */4, quarters are 100, eights 50, sixteenths 25)
  //First index is a placeholder. eventtimes[] has an extra time, at which it loops back to index 1.
  int eventtimes[30] = {0,  0, 15, 75, 90,100,115,175,190,225,240,275,290,300,315,400,415,475,490,500,515,575,590,625,640,675,690,700,715,800};
  int eventtypes[29] = {0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0};
  int curevent = 0; //Current event (index in arrays above)
  int cureventnote = 0; //Current note in the event (to support cycling through a chord). 1 for single note. 0 when not playing a note presently.
  float transpose = 1;
  unsigned long eventstart = 0; //0 when not playing a song presently.
  //we will only keep track of time since last event, thus bpm may vary due to rounding errors, but millis() rollover will only cause one event skip (I think?).
public:
  Song(int zero) {}
  void play(){
    check(true); //the true means this starts the song
  } //end void play
  void check(bool start){ if(start || curevent){ //if song starting or playing
    unsigned long mils = millis();
    //Are we moving to a new event?
    int neweventdiff = (eventtimes[curevent+1]-eventtimes[curevent])*(600/bpm); //*(600/bpm) converts diff in centibeats to milliseconds
    if(start || mils-eventstart >= neweventdiff){
      if(start) eventstart = mils; //true start of event
      else eventstart = eventstart + neweventdiff; //set start of event relative to start of last, for better timekeeping
      curevent++; //Next event
      if(curevent+1 == 30){ curevent=1; } //Have we looped? Go back to first event //eventtimes.size
    }
    //Should we do anything about the current event?
    int neweventnote = (((mils-eventstart)%120)/40)+1; //a cycle of 3 every 120 ms (40ms/ea), based on current time
    switch(eventtypes[curevent]){ //The type of note/chord/stop
      case 1: //C Major chord, cycle of 523.25, 659.25, 784 Hz
        if(cureventnote!=neweventnote) {
          cureventnote = neweventnote;
          tone(pin, (cureventnote==1? 523.25*transpose: (cureventnote==2? 659.25*transpose: 784*transpose)), dur);
        }
        break;
      case 2: //F Major chord, cycle of 523.25, 698.45, 880 Hz
        if(cureventnote!=neweventnote) {
          cureventnote = neweventnote;
          tone(pin, (cureventnote==1? 523.25*transpose: (cureventnote==2? 698.45*transpose: 880*transpose)), dur);
        }
        break;
      case 0: //Stop - turn off sound, if playing
        if(cureventnote!=0){ cureventnote=0; noTone(pin); } break;
      default: break;
    }
  }} //end void check if song playing
  void stop(){
    noTone(pin);
    curevent = 0;
    cureventnote = 0;
    eventstart = 0;
  } //end void stop
}; //end class Song