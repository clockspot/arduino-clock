#include "Arduino.h"
#include "SongPlayer.h"

//Define the songs.
//Each of the sNt and sNp arrays, one for each song, needs to be declared in SongPlayer.h.
//sNt contains that song's event times (like baby MIDI) in centibeats. Last event is the song end (no pitch).
//sNp contains each event's corresponding note pitch in piano key number, or 0 for silence. Examples:
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
const int SongPlayer::s5t[57]  = {0,  5, 10, 15,  75, 80, 85, 90,100,105,110,115, 175,180,185,190, 225,230,235,240, 275,280,285,290,300,305,310,315, 400,405,410,415, 475,480,485,490,500,505,510,515, 575,580,585,590, 625,630,635,640, 675,680,685,690,700,705,710,715, 800};
const int SongPlayer::s5p[56] = {52, 56, 59,  0,  52, 56, 59,  0, 52, 56, 59,  0,  52, 56, 59,  0,  52, 56, 59,  0,  52, 56, 59,  0, 52, 56, 59,  0,  52, 57, 61,  0,  52, 57, 61,  0, 52, 57, 61,  0,  52, 57, 61,  0,  52, 57, 61,  0,  52, 57, 61,  0, 52, 57, 61,  0};

const int  SongPlayer::songSize[6]    = {  3,  3,  5,  7,  9, 57}; //songs' size - since can't use sizeof() via array pointer
//Pointers to song bpms, times, and pitches
const int  SongPlayer::songBPM[6]     = {600,600,600,600,600, 80}; //songs' beats per minute. 600 for centibeats=milliseconds
const int* SongPlayer::songTimes[6]   = {s0t,s1t,s2t,s3t,s4t,s5t};
const int* SongPlayer::songPitches[6] = {s0p,s1p,s2p,s3p,s4p,s5p};

void SongPlayer::play(int song, int plays){
  cursong = song;
  curplays = plays;
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
      Serial.print(F("Starting song ")); Serial.print(cursong,DEC); //Serial.print(F(" at ")); Serial.println(cureventstart,DEC);
      Serial.print(F(" with curplays=")); Serial.print(curplays,DEC);
    } else { //continuing song
      cureventstart = cureventstart + neweventdiff; //set start of event relative to start of last, for better timekeeping
      curevent++; //next event
      //Serial.print(F("Event ")); Serial.print(curevent,DEC); Serial.print(F(" at ")); Serial.println(cureventstart,DEC);
    }
    Serial.print(F("curevent now ")); Serial.println(curevent,DEC);
    if(curevent+1 == songSize[cursong]){ //If this is the last event in the song
      curplays--;
      Serial.print(F("curplays now ")); Serial.println(curplays,DEC);
      if(curplays>0) {
        curevent=0;
        //Serial.println(F("Starting over"));
      } else {
        //Serial.println(F("Stopping"));
        stop();
      }
    }
    if(curplays){ //if song is still playing
      if(songPitches[cursong][curevent]>0) tone(PIEZO_PIN, getHz(songPitches[cursong][curevent]), DUR); //note start
      else noTone(PIEZO_PIN); //note stop
    }
  }
}}

void SongPlayer::stop(){
  noTone(PIEZO_PIN);
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

void loop(){
  piezo.check(false); //check for new events in the currently playing song
}
*/