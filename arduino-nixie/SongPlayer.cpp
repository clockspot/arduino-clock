#include "Arduino.h"
#include "SongPlayer.h"

//Define the songs.
//Each of the sNt and sNp arrays, one for each song, needs to be declared in SongPlayer.h.
//sNt contains that song's event times (like baby MIDI) in centibeats. Last event is the song end (no pitch).
//sNp contains each event's corresponding note pitch (<100 in piano key number, >=100 in Hz), or 0 for silence. Examples:
//Pitch:      C4 E4 A4(440Hz)  C5 E5 A5   C6 E6 A6   C7 E7 A7   C8
//Piano key:  40 44 49         52 56 61   64 68 73   76 80 85   88

//0. Short (125ms) beep with (optional) 1s loop
const int SongPlayer::s0t[3] = { 0,125,1000}; //
const int SongPlayer::s0p[2] = {76,  0}; //pitch of note, or 0 to stop. times array has one extra value indicating end.
//1. Long (500ms) beep with 1s loop
const int SongPlayer::s1t[3] = { 0,500,1000};
const int SongPlayer::s1p[2] = {76,  0};
//2. 2x beep with 1s loop
const int SongPlayer::s2t[5] = { 0,125,250,375,1000};
const int SongPlayer::s2p[4] = {76,  0, 76,  0};
//3. 3x beep
const int SongPlayer::s3t[7] = { 0,125,250,375,500,625,1000};
const int SongPlayer::s3p[6] = {76,  0, 76,  0, 76,  0};
//4. 4x beep
const int SongPlayer::s4t[9] = { 0, 62,125,187,250,312,375,437,1000};
const int SongPlayer::s4p[8] = {76,  0, 76,  0, 76,  0, 76,  0};
//5. Woody's chorus part from Vulfpeck "1612", trilled
const int SongPlayer::s5t[57] = { 0,  5, 10, 15,  75, 80, 85, 90,100,105,110,115, 175,180,185,190, 225,230,235,240, 275,280,285,290,300,305,310,315, 400,405,410,415, 475,480,485,490,500,505,510,515, 575,580,585,590, 625,630,635,640, 675,680,685,690,700,705,710,715, 800};
const int SongPlayer::s5p[56] = {52, 56, 59,  0,  52, 56, 59,  0, 52, 56, 59,  0,  52, 56, 59,  0,  52, 56, 59,  0,  52, 56, 59,  0, 52, 56, 59,  0,  52, 57, 61,  0,  52, 57, 61,  0, 52, 57, 61,  0,  52, 57, 61,  0,  52, 57, 61,  0,  52, 57, 61,  0, 52, 57, 61,  0};
//6. The pips
const int SongPlayer::s6t[12] = {   0,100,1000,1100,2000,2100,3000,3100,4000,4100,5000,5500};
const int SongPlayer::s6p[12] = {1000,  0,1000,   0,1000,   0,1000,   0,1000,   0,1000,   0};

const int  SongPlayer::songSize[7]    = {  3,  3,  5,  7,  9, 57, 12};
  //songs' size - since can't use sizeof() via array pointer
//Pointers to song bpms, times, and pitches
const int  SongPlayer::songBPM[7]     = {600,600,600,600,600, 80,600};
  //songs' beats per minute. 600 for centibeats=milliseconds
const int* SongPlayer::songTimes[7]   = {s0t,s1t,s2t,s3t,s4t,s5t,s6t};
const int* SongPlayer::songPitches[7] = {s0p,s1p,s2p,s3p,s4p,s5p,s6p};

//For now, the selectable alarm and strike sounds can come straight from these arrays.
//TODO in future, we may want to separate it out so, for example, some "strikes" could support half or quarter hour songs
//of all songs, which can be used as alarms, and in what order?
const int SongPlayer::alarmSongs[6]     = {0,1,2,3,4,5};
//of all songs, which can be used as hourly strikes, and in what order? And which are allowed to count the hour?
const int SongPlayer::strikeSongs[3]    = {1,2,6};
const int SongPlayer::strikeCanCount[3] = {1,0,0};

void SongPlayer::play(int song, int type, int plays){
  //Check song id to make sure it's within bounds of the song type list first
  if(song<0) song==1;
  if(song>=(type==1? sizeof(alarmSongs)/sizeof(alarmSongs[0]):
           (type==2? sizeof(strikeSongs)/sizeof(strikeSongs[0]):
                     sizeof(songSize)/sizeof(songSize[0])))) song==1;
  cursong = (type==1? alarmSongs[song]: (type==2? strikeSongs[song]: song));
  curplays = (type==2? (strikeCanCount[song]? plays: 1): plays); //if strike, see if the song is allowed to repeat
  curevent = 0;
  cureventstart = 0;
  check(true); //the true means this starts the song
} //end fn play

void SongPlayer::check(bool start){ if(curplays){ //if song playing
  unsigned long mils = millis();
  //Are we moving to a new event? (convert event timings from centibeats to milliseconds)
  unsigned long neweventdiff = (songTimes[cursong][curevent+1]-songTimes[cursong][curevent])*(600 / songBPM[cursong]);
  if(start || mils-cureventstart >= neweventdiff){
    if(start) { //starting song
      cureventstart = mils;
      curevent = 0; //first event
    } else { //continuing song
      cureventstart = cureventstart + neweventdiff; //set start of event relative to start of last, for better timekeeping
      curevent++; //next event
    }
    if(curevent+1 == songSize[cursong]){ //If this is the last event in the song
      curplays--; if(curplays>0) curevent=0; else stop(); //really it'll stop itself
    }
    if(curplays){ //if song is still playing
      if(songPitches[cursong][curevent]>0) tone(piezoPin, (songPitches[cursong][curevent]>=100? songPitches[cursong][curevent]: getHz(songPitches[cursong][curevent])), DUR); //note start - if pitch>=100 it's Hz, else getHz from piano key number
      else noTone(piezoPin); //note stop
    }
  }
}}

void SongPlayer::stop(){
  noTone(piezoPin);
  cursong = 0;
  curplays = 0;
  curevent = 0;
  cureventstart = 0;
}

static word SongPlayer::getHz(byte note){
  //Given a piano key note, return frequency
  char relnote = note-49; //signed, relative to concert A
  float reloct = relnote/12.0; //signed
  unsigned int mult = 440*pow(2,reloct);
  return mult;
}

//Sample code for main
/*
#include "SongPlayer.h"

SongPlayer piezo;

void setup(){
  Serial.begin(9600);
  piezo.play(3,3);
}

unsigned long lastMillis = 0;
int testsPlayed = 0;
void loop(){
  unsigned long mils = millis();
  if(testsPlayed==0)              { testsPlayed++; piezo.play(0); } //0sec: single beep
  if(testsPlayed==1 && mils>3000) { testsPlayed++; piezo.play(1,0,2); } //3sec: 2x long beeps
  if(testsPlayed==2 && mils>6000) { testsPlayed++; piezo.play(0,1,2); } //6sec: 2x long beeps, addressed as alarm
  if(testsPlayed==3 && mils>9000) { testsPlayed++; piezo.play(1,1,2); } //9sec: 2x double beep, addressed as alarm
  if(testsPlayed==4 && mils>12000) { testsPlayed++; piezo.play(2,1,2); } //12sec: 2x triple beep, addressed as alarm
  if(testsPlayed==5 && mils>15000) { testsPlayed++; piezo.play(3,1,2); } //15sec: 2x quad beep, addressed as alarm
  if(testsPlayed==6 && mils>18000) { testsPlayed++; piezo.play(4,1,1); } //18sec: 1612, addressed as alarm
  if(testsPlayed==7 && mils>25000) { testsPlayed++; piezo.play(0,2,2); } //25sec: long beep, addressed as strike at 2pm
  if(testsPlayed==8 && mils>28000) { testsPlayed++; piezo.play(1,2,2); } //28sec: double beep, addressed as alarm at 2pm
  if(testsPlayed==9 && mils>31000) { testsPlayed++; piezo.play(2,2,2); } //31sec: pips, addressed as alarm at 2pm
  piezo.check(false); //check for new events in the currently playing song
}

//SongPlayer test takes 23% of memory on its own, and 31% of dynamic memory - why???
//Everything takes 59% of memory and 36% of dynamic memory

*/