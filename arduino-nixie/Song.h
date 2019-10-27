class Song {
  //Makes the piezo beeper play patterns and songs.
  
  int pin = 10; //pin the piezo is connected to
  int dur = 10000; //max note length in millis
  
  /*
  Define songs
  bpm is beats per minute
  event[] is an array of times to start and stop notes (like a baby MIDI), in centibeats.
    (e.g. in x/4, quarters are 100, eights 50, sixteenths 25). When bpm is 600, centibeats = milliseconds.
    Has one more value than pitch[] does, to represent the time when the song should stop or loop.
  pitch[] is an array of note pitches, expressed in piano key, to start; or 0 to stop.
    Piano notes (A4=440Hz. I think C7 is the beeper's native note)
    C4 E4 A4  C5 E5 A5  C6 E6 A6  C7 E7 A7  C8
    40 44 49  52 56 61  64 68 73  76 80 85  88
  */
  
  //0. Short (125ms) beep with (optional) 1s loop
  int bpmShort = 600;
  int eventShort[3] = { 0,125,1000};
  int pitchShort[2] = {76,  0};
  //1. Long (500ms) beep with 1s loop
  int bpmLong = 600;
  int eventLong[3] = { 0,500,1000};
  int pitchLong[2] = {76,  0};
  //2. 2x beep with 1s loop
  int bpm2x = 600;
  int event2x[5] = { 0,125,250,375,1000};
  int pitch2x[4] = {76,  0, 76,  0};
  //3. 4x beep
  int bpm4x = 600;
  int event4x[9] = { 0, 62,125,187,250,312,375,437,1000};
  int pitch4x[8] = {76,  0, 76,  0, 76,  0, 76,  0};
  //4. Woody's chorus part from "1612", trilled
  int bpmVulf = 80;
  int eventVulf[62] = {0,   0,  5, 10, 15,  75, 80, 85, 90,100,105,110,115, 175,180,185,190, 225,230,235,240, 275,280,285,290,300,305,310,315, 400,405,410,415, 475,480,485,490,500,505,510,515, 575,580,585,590, 625,630,635,640, 675,680,685,690,700,705,710,715, 800};
  int pitchVulf[61] = {0,  52, 56, 59,  0,  52, 56, 59,  0, 52, 56, 59,  0,  52, 56, 71,  0,  52, 56, 71,  0,  52, 56, 71,  0, 52, 56, 71,  0,  52, 57, 61,  0,  52, 57, 61,  0, 52, 57, 61,  0,  52, 57, 61,  0,  52, 57, 61,  0,  52, 57, 61,  0, 52, 57, 61,  0};
  
  //Pointers to address songs programmatically. When adding songs, add them to this list.
  int *bpm[5] = { &bpmShort, &bpmLong, &bpm2x, &bpm4x, &bpmVulf };
  int *event[5] = { &eventShort, &eventLong, &event2x, &event4x, &eventVulf };
  int *pitch[5] = { &pitchShort, &pitchLong, &pitch2x, &pitch4x, &pitchVulf };
  
  //Variables 
  int cursong = 0; 
  int cursongend = 0; //0 for stop, 1 for loop
  int curevent = 0; //Current event (index in arrays above)
  unsigned long eventstart = 0; //0 when not playing a song presently.
  //we will only keep track of time since last event, thus bpm may vary due to rounding errors, but millis() rollover will only cause one event skip (I think?).
public:
  Song(int zero){}
  void play(songId,doLoop){
    cursong = songId; //index in the pointer arrays
    cursongend = doLoop;
    check(true); //the true means this starts the song
  } //end void play
  void check(bool start){ if(start || eventstart){ //if song starting or playing
    unsigned long mils = millis();
    //Are we moving to a new event?
    unsigned long neweventdiff = (*event[songId][curevent+1]-*event[songId][curevent])*(600/bpm); //*(600/bpm) converts diff in centibeats to milliseconds
    if(start || mils-eventstart >= neweventdiff){
      if(start) { //starting song
        eventstart = mils;
        curevent = 0; //first event
      } else { //continuing song
        eventstart = eventstart + neweventdiff; //set start of event relative to start of last, for better timekeeping
        curevent++; //next event
      }
      if(curevent+1 == sizeof(*event[songId])/sizeof(*event[songId][0])){ //Is this the last event?
        if(songend) curevent=0; else stop(); //Loop or stop
      } else {
        if(*pitch[songId][curevent]>0) tone(pin, getHz(*pitch[songId][curevent]), dur); //start note
        else noTone(pin); //stop note
      }
    }
  }} //end void check if song playing
  void stop(){
    noTone(pin);
    cursong = 0;
    cursongend = 0;
    curevent = 0;
    eventstart = 0;
  } //end void stop
  word getHz(byte note){
    //Given a piano key note, return frequency
    char relnote = note-49; //signed, relative to concert A
    float reloct = relnote/12.0; //signed
    word mult = 440*pow(525761,reloct);
    return mult;
  } //end word getHz
}; //end class Song