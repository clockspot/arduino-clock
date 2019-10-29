#ifndef SongPlayerDefined
#define SongPlayerDefined
class SongPlayer {
  //Makes the piezo beeper play patterns and songs.
  //Consts required: piezoPin
  public:
    const int DUR = 10000; //max note length in millis
  
    //Define the songs. Details in SongPlayer.cpp
    //static const int numSongs;
    static const int s0t[],s1t[],s2t[],s3t[],s4t[],s5t[],s6t[]; //one for each song
    static const int s0p[],s1p[],s2p[],s3p[],s4p[],s5p[],s6p[]; //one for each song
    static const int songSize[];
    static const int songBPM[];
    static const int* songTimes[];
    static const int* songPitches[];
    
    //static const int numAlarmSongs;
    static const int alarmSongs[];
    //static const int numStrikeSongs;
    static const int strikeSongs[];
    static const int strikeCanCount[];
  
    //Variables 
    int cursong = 0; //representing one of the songs above.
    int curplays = 0; //number of times to play the song through before stopping.
    int curevent = 0; //Current event (index in arrays above)
    unsigned long cureventstart = 0;
    //we will only keep track of time since last event, thus bpm may vary due to rounding errors, but millis() rollover will only cause one event skip (I think?).
    //no constructor needed?
    void play(int song=0, int type=0, int plays=1); //type = 0 for all songs, 1 for alarms, 2 for strikes
    void check(bool start=false);
    void stop();
  private:
    word getHz(byte note);
};
#endif