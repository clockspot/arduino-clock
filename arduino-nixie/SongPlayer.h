#ifndef SongPlayerDefined
#define SongPlayerDefined
class SongPlayer {
  //Makes the piezo beeper play patterns and songs.
  public:
    const int PIEZO_PIN = 10; //pin the piezo is connected to
    const int DUR = 10000; //max note length in millis
  
    //Define the songs. Details in SongPlayer.cpp
    static const int s0t[],s1t[],s2t[],s3t[],s4t[],s5t[]; //one for each song
    static const int s0p[],s1p[],s2p[],s3p[],s4p[],s5p[]; //one for each song
    static const int songSize[];
    static const int songBPM[];
    static const int* songTimes[];
    static const int* songPitches[];
  
    //Variables 
    int cursong = 0; //representing one of the songs above.
    int curplays = 120; //number of times to play the song through before stopping. (120 = 2 mins at 1sec loop)
    int curevent = 0; //Current event (index in arrays above)
    unsigned long cureventstart = 0;
    //we will only keep track of time since last event, thus bpm may vary due to rounding errors, but millis() rollover will only cause one event skip (I think?).
    //no constructor needed?
    void play(int song, int plays);
    void check(bool start);
    void stop();
  private:
    word getHz(byte note);
};
#endif