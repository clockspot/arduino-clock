#ifdef NETWORK //only compile when requested (when included in main file)
#ifndef NETWORK_SRC //include once only
#define NETWORK_SRC

#ifndef __AVR__ //TODO better sensor
//do stuff for wifinina
#define NETWORK_OK
  
//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#include <WiFiNINA.h>
#include <WiFiUdp.h>

String wssid = "Riley";
String wpass = "5802301644"; //wpa pass or wep key
byte wki = 0; //wep key index - 0 if using wpa
//TODO how to persistent store this - one byte at a time up to the max

unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server TODO make configurable
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

WiFiServer server(80);

const unsigned long ADMIN_TIMEOUT = 3600000; //120000; //two minutes
const unsigned long NTPOK_THRESHOLD = 3600000; //if no sync within 60 minutes, the time is considered stale

//TODO hide second 58 when no wifi; 59 when NTP is bad (old or 0), and retry every minute 0-5
        

//Declare a few functions so I can put them out of order
//Placed here so I can avoid making header files for the moment
void cueNTP();
void checkNTP();

int statusLast;
void checkForWiFiStatusChange(){
  // if(WiFi.status()!=statusLast){
  //   Serial.print(millis()); Serial.print(F(" WiFi status has changed to "));
  //   statusLast = WiFi.status();
  //   switch(statusLast){
  //     case WL_IDLE_STATUS: Serial.print(F("WL_IDLE_STATUS")); break;
  //     case WL_NO_SSID_AVAIL: Serial.print(F("WL_NO_SSID_AVAIL")); break;
  //     case WL_SCAN_COMPLETED: Serial.print(F("WL_SCAN_COMPLETED")); break;
  //     case WL_CONNECTED: Serial.print(F("WL_CONNECTED")); break;
  //     case WL_CONNECT_FAILED: Serial.print(F("WL_CONNECT_FAILED")); break;
  //     case WL_CONNECTION_LOST: Serial.print(F("WL_CONNECTION_LOST")); break;
  //     case WL_DISCONNECTED: Serial.print(F("WL_DISCONNECTED")); break;
  //     case WL_AP_LISTENING: Serial.print(F("WL_AP_LISTENING")); break;
  //     case WL_AP_CONNECTED: Serial.print(F("WL_AP_CONNECTED")); break;
  //     default: break;
  //   }
  //   Serial.print(F(" (")); Serial.print(WiFi.status()); Serial.println(F(")"));
  // }
}

void networkStartWiFi(){
  WiFi.end(); //if AP is going, stop it
  if(wssid==F("")) return; //don't try to connect if there's no creds
  checkForWiFiStatusChange(); //just for serial logging
  blankDisplay(0,5,false); //I'm guessing if it hangs, nixies won't be able to display anyway
  //Serial.println(); Serial.print(millis()); Serial.print(F(" Attempting to connect to SSID: ")); Serial.println(wssid);
  if(wki) WiFi.begin(wssid.c_str(), wki, wpass.c_str()); //WEP - hangs while connecting
  else WiFi.begin(wssid.c_str(), wpass.c_str()); //WPA - hangs while connecting
  if(WiFi.status()==WL_CONNECTED){ //did it work?
    // Serial.print(millis()); Serial.println(F(" Connected!"));
    // Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
    // Serial.print(F("Signal strength (RSSI):")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
    // Serial.print(F("Access the admin page by browsing to http://")); Serial.println(WiFi.localIP());
    server.begin(); Udp.begin(localPort); cueNTP();
  }
  //else Serial.println(F(" Wasn't able to connect."));
  updateDisplay();
  checkForWiFiStatusChange(); //just for serial logging
} //end fn startWiFi

void networkStartAP(){
  WiFi.end(); //if wifi is going, stop it
  checkForWiFiStatusChange(); //just for serial logging
  //Serial.println(); Serial.print(millis()); Serial.println(F(" Creating access point"));
  if(WiFi.beginAP("Clock")==WL_AP_LISTENING){ //Change "beginAP" if you want to create an WEP network
    //Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
    //by default the local IP address of will be 192.168.4.1 - override with WiFi.config(IPAddress(10, 0, 0, 1));
    WiFi.config(IPAddress(7,7,7,7));
    //Serial.print(F("Access the admin page by browsing to http://")); Serial.println(WiFi.localIP());
    //server.begin() was formerly here
    server.begin();
  }
  //else Serial.println(F(" Wasn't able to create access point."));
  checkForWiFiStatusChange(); //just for serial logging
} //end fn startAP

void networkDisconnectWiFi(){
  //Serial.println(F("Disconnecting WiFi - will try to connect at next NTP sync time"));
  WiFi.end();
}

bool ntpCued = false;
unsigned int ntpStartLast = -3000;
bool ntpGoing = 0;
unsigned int ntpSyncLast = 0;

void cueNTP(){
  //We don't want to let other code startNTP() directly since it's asynchronous, and that other code may delay the time until we can check the result
  ntpCued = true;
}

void startNTP(){ //Called at intervals to check for ntp time
  if(wssid==F("")) return; //don't try to connect if there's no creds
  if(WiFi.status()!=WL_CONNECTED) networkStartWiFi(); //in case it dropped
  if(WiFi.status()!=WL_CONNECTED) return;
  if(ntpGoing && millis()-ntpStartLast < 3000) return; //if a previous request is going, do not start another until at least 3sec later
  ntpGoing = 1;
  ntpStartLast = millis();
  Udp.flush(); //in case of old data
  //Udp.stop() was formerly here
  //Serial.println(); Serial.print(millis()); Serial.println(F(" Sending UDP packet to NTP server."));
  memset(packetBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(timeServer, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  checkNTP(); //in case it comes back quickly enough
} //end fn startNTP

unsigned long ntpTime = 0; //When this is nonzero, it means we have captured a time and are waiting to set the clock until the next full second, in order to achieve subsecond setting precision (or close to - it'll be behind by up to the loop time, since we aren't simply using delay() in order to keep the nixie display going).
unsigned  int ntpMils = 0;
void checkNTP(){ //Called on every cycle to see if there is an ntp response to handle
  if(ntpGoing){
    //If we are waiting for a packet that hasn't arrived, wait for the next cycle
    if(!Udp.parsePacket()) return;
    // We've received a packet, read the data from it
    ntpSyncLast = millis();
    unsigned int requestTime = ntpSyncLast-ntpStartLast;
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
    //https://forum.arduino.cc/index.php?topic=526792.0
    //epoch in earlier bits? needed after 2038
    ntpTime = (packetBuffer[40] << 24) | (packetBuffer[41] << 16) | (packetBuffer[42] << 8) | packetBuffer[43];
    unsigned long ntpFrac = (packetBuffer[44] << 24) | (packetBuffer[45] << 16) | (packetBuffer[46] << 8) | packetBuffer[47];
    ntpMils = (int32_t)(((float)ntpFrac / UINT32_MAX) * 1000);
    
    //Account for the request time
    ntpMils += requestTime/2;
    if(ntpMils>=1000) { ntpMils -= 1000; ntpTime++; }
    
    // Serial.print(F("NTP time: "));
    // Serial.print(ntpTime,DEC);
    // Serial.print(F("."));
    // Serial.print(ntpMils,DEC);
    // Serial.print(F(" ±"));
    // Serial.print(requestTime,DEC);
  
    //Unless the mils are bang on, we'll wait to set the clock until the next full second.
    if(ntpMils>0) ntpTime++;
    
    // Serial.print(F(" - set to "));
    // Serial.print(ntpTime,DEC);
    // if(ntpMils==0) Serial.print(F(" immediately"));
    // else { Serial.print(F(" after ")); Serial.print(1000-ntpMils,DEC); }
    // Serial.println();

    Udp.flush(); //in case of extraneous(?) data
    //Udp.stop() was formerly here
    ntpGoing = 0;
  }
  if(!ntpGoing){
    //If we are waiting to start, do it
    if(ntpCued){ startNTP(); ntpCued=false; return; }
    //If we are not waiting to set, do nothing
    if(!ntpTime) return;
    //If we are waiting to set, but it's not time, wait for the next cycle
    if(ntpMils!=0 && (unsigned long)(millis()-ntpSyncLast)<(1000-ntpMils)) return;
    //else it's time!

    //Convert unix timestamp to UTC date/time
    //TODO this assumes epoch 0, which is only good til 2038, I think!
    ntpTime -= 3155673600; //from 1900 to 2000, assuming epoch 0
    unsigned long ntpPart = ntpTime;
    int y = 2000;
    while(1){ //iterate to find year
      unsigned long yearSecs = daysInYear(y)*86400;
      if(ntpPart > yearSecs){
        ntpPart-=yearSecs; y++;
      } else break;
    }
    byte m = 1;
    while(1){ //iterate to find month
      unsigned long monthSecs = daysInMonth(y,m)*86400;
      if(ntpPart > monthSecs){
        ntpPart-=monthSecs; m++;
      } else break;
    }
    byte d = 1+(ntpPart/86400); ntpPart %= 86400;
    int hm = ntpPart/60; //mins from midnight
    byte s = ntpPart%60;
  
    //Take UTC date/time and apply standard offset
    //which involves checking for date rollover
    //eeprom loc 14 is UTC offset in quarter-hours plus 100 - range is 52 (-12h or -48qh, US Minor Outlying Islands) to 156 (+14h or +56qh, Kiribati)
    int utcohm = (readEEPROM(14,false)-100)*15; //utc offset in mins from midnight
    if(hm+utcohm<0){ //date rolls backward
      hm = hm+utcohm+1440; //e.g. -1 to 1439 which is 23:59
      d--; if(d<1){ m--; if(m<1){ y--; m=12; } d=daysInMonth(y,m); } //month or year rolls backward
    } else if(hm+utcohm>1439){ //date rolls forward
      hm = (hm+utcohm)%1440; //e.g. 1441 to 1 which is 00:01
      d++; if(d>daysInMonth(y,m)){ m++; if(m>12){ y++; m=1; } d=1; } //month or year rolls forward
    } else hm += utcohm;
  
    //then check DST at that time (setting DST flag), and add an hour if necessary
    //which involves checking for date rollover again (forward only)
    //TODO this may behave unpredictably from 1–2am on fallback day since that occurs twice - check to see whether it has been applied already per the difference from utc
    if(isDSTByHour(y,m,d,hm/60,true)){
      if(hm+60>1439){ //date rolls forward
        hm = (hm+60)%1440; //e.g. 1441 to 1 which is 00:01
        d++; if(d>daysInMonth(y,m)){ m++; if(m>12){ y++; m=1; } d=1; } //month or year rolls forward
      } else hm += 60;
    }
  
    //finally set the rtc
    rtcSetDate(y, m, d, dayOfWeek(y,m,d));
    rtcSetTime(hm/60,hm%60,s);
    millisAtLastCheck = 0; //see ms()
    calcSun();
  
    // Serial.print(F("RTC set to "));
    // Serial.print(rtcGetYear(),DEC); Serial.print(F("-"));
    // if(rtcGetMonth()<10) Serial.print(F("0")); Serial.print(rtcGetMonth(),DEC); Serial.print(F("-"));
    // if(rtcGetDate()<10) Serial.print(F("0")); Serial.print(rtcGetDate(),DEC); Serial.print(F(" "));
    // if(rtcGetHour()<10) Serial.print(F("0")); Serial.print(rtcGetHour(),DEC); Serial.print(F(":"));
    // if(rtcGetMinute()<10) Serial.print(F("0")); Serial.print(rtcGetMinute(),DEC); Serial.print(F(":"));
    // if(rtcGetSecond()<10) Serial.print(F("0")); Serial.print(rtcGetSecond(),DEC);
    // Serial.println();
    
    ntpTime = 0; ntpMils = 0; //no longer waiting to set
    updateDisplay();
  }
} //end fn checkNTP

bool networkNTPOK(){
  //Serial.print(F("NTP is ")); Serial.print(millis()-ntpSyncLast < NTPOK_THRESHOLD?F("OK: "):F("stale: ")); Serial.print(millis()-ntpSyncLast); Serial.print(F("ms old (vs limit of ")); Serial.print(NTPOK_THRESHOLD); Serial.print(F("ms)")); Serial.println();
  return (millis()-ntpSyncLast < NTPOK_THRESHOLD);
}

unsigned long adminInputLast = 0; //for noticing when the admin page hasn't been interacted with in 2 minutes, so we can time it (and AP if applicable) out

void networkStartAdmin(){
  adminInputLast = millis();
  if(WiFi.status()!=WL_CONNECTED){
    networkStartAP();
    tempValDispQueue[0] = 7777; //display to user
    for(char i=1; i<=3; i++) tempValDispQueue[i] = 0;
    //Serial.println(F("Admin started at 7.7.7.7"));
  } else { //use existing wifi
    IPAddress theip = WiFi.localIP();
    for(char i=0; i<=3; i++) tempValDispQueue[i] = theip[i]; //display to user
    //Serial.print(F("Admin started at "));
    //Serial.println(theip);
  }
  updateDisplay();
}
void networkStopAdmin(){
  Serial.println(F("stopping admin"));
  adminInputLast = 0; //TODO use a different flag from adminInputLast
  if(WiFi.status()!=WL_CONNECTED) networkStartWiFi();
}

//unsigned long debugLast = 0;
void checkClients(){
  // if((unsigned long)(millis()-debugLast)>=1000) { debugLast = millis();
  //   Serial.print("Hello ");
  //   Serial.println(WiFi.status());
  // }
  //if(WiFi.status()!=WL_CONNECTED && WiFi.status()!=WL_AP_CONNECTED) return;
  if(adminInputLast && (unsigned long)(millis()-adminInputLast)>=ADMIN_TIMEOUT) networkStopAdmin();
  WiFiClient client = server.available();
  if(client) {
    if(adminInputLast==0) {
      client.flush(); client.stop();
      //Serial.print(F("Got a client but ditched it because last admin input was over ")); Serial.print(ADMIN_TIMEOUT); Serial.println(F("ms ago."));
      return;
    }
    else {
      //Serial.print(F("Last admin input was ")); Serial.print(millis()-adminInputLast); Serial.print(F("ms ago which is under the limit of ")); Serial.print(ADMIN_TIMEOUT); Serial.println(F("ms."));
    }
    
    adminInputLast = millis();
    
    String currentLine = ""; //we'll read the data from the client one line at a time
    int requestType = 0;
    bool newlineSeen = false;

    if(client.connected()){
      while(client.available()){ //if there's bytes to read from the client
        char c = client.read();
        //Serial.write(c); //DEBUG
        
        if(c=='\n') newlineSeen = true;
        else {
          if(newlineSeen){ currentLine = ""; newlineSeen = false; } //if we see a newline and then something else: clear current line
          currentLine += c;
        }

        //Find the request type and path from the first line.
        if(!requestType){
          if(currentLine=="GET / ") { requestType = 1; break; } //Read no more. We'll render out the page.
          if(currentLine=="POST / ") requestType = 2; //We'll keep reading til the last line.
          if(c=='\n') break; //End of first line without matching the above: invalid request, return nothing.
        }
        
      } //end whie client available
    } //end if client connected
    
    if(requestType){
      // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
      // and a content-type so the client knows what's coming, then a blank line:
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Access-Control-Allow-Origin:*");
      client.println();
      if(requestType==1){ //get
        
        //Re hiding irrelevant options, see also fnOptScroll in main code
        
        client.print(F("<!DOCTYPE html><html><head><title>Clock Settings</title><style>body { background-color: #eee; color: #222; font-family: system-ui, -apple-system, sans-serif; font-size: 18px; line-height: 1.3em; margin: 1.5em; position: absolute; } a { color: #33a; } ul { padding-left: 9em; text-indent: -9em; list-style: none; margin-bottom: 4em; } ul li { margin-bottom: 0.8em; } ul li * { text-indent: 0; padding: 0; } ul li label:first-child { display: inline-block; width: 8em; text-align: right; padding-right: 1em; font-weight: bold; } ul li.nolabel { margin-left: 9em; } ul li h3 { display: inline-block; margin: 1em 0 0; } input[type='text'],input[type='number'],input[type='submit'],select { border: 1px solid #999; margin: 0.2em 0; padding: 0.1em 0.3em; font-size: 1em; font-family: system-ui, -apple-system, sans-serif; } @media only screen and (max-width: 550px) { ul { padding-left: 0; text-indent: 0; } ul li label:first-child { display: block; width: auto; text-align: left; padding: 0; } ul li.nolabel { margin-left: 0; }} .saving { color: #66d; } .ok { color: #3a3; } .error { color: #c53; } .explain { font-size: 0.85em; line-height: 1.3em; color: #666; } @media (prefers-color-scheme: dark) { body { background-color: #222; color: #ddd; } a { color: white; } #result { background-color: #373; color: white; } input[type='text'],input[type='number'],select { background-color: black; color: #ddd; } .explain { color: #999; } }</style><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'></head><body><h2 style='margin-top: 0;'>Clock Settings</h2><p id='loading'>Loading&hellip;<br/><br/>If page doesn't appear in a few seconds, <a href=\"#\" onclick=\"location.reload(); return false;\">refresh</a>.</p><div id='content' style='display: none;'><ul>"));
        
        client.print(F("<li><h3>General</h3></li>"));
        
        client.print(F("<li><label>Version</label>")); //TODO upload new built version?
          client.print(vMajor,DEC);
          client.print(F("."));
          client.print(vMinor,DEC);
          client.print(F("."));
          client.print(vPatch,DEC);
          if(vDev) //don't link directly to anything, just the project
            client.print(F("-dev (<a href='https://github.com/clockspot/arduino-nixie' target='_blank'>details</a>)"));
          else { //link directly to the release of this version
            client.print(F(" (<a href='https://github.com/clockspot/arduino-nixie/releases/tag/v")); //TODO needs fix after rename
            client.print(vMajor,DEC);
            client.print(F("."));
            client.print(vMinor,DEC);
            client.print(F("."));
            client.print(vPatch,DEC);
            client.print(F("' target='_blank'>details</a>)"));
          }
          client.print(F("</li>"));
        
        //Wi-Fi, NTP, and UTC offset are always relevant given network
        client.print(F("<li><label>Wi-Fi</label><form id='wform' style='display: inline;' onsubmit='save(this); return false;'><select id='wtype' onchange='wformchg()'><option value=''>None</option><option value='wpa'>WPA</option><option value='wep'>WEP</option></select><span id='wa'><br/><input type='text' id='wssid' name='wssid' placeholder='SSID (Network Name)' autocomplete='off' onchange='wformchg()' onkeyup='wformchg()' value='")); String wssid2 = wssid; wssid2.replace("'","&#39;"); client.print(wssid2); client.print(F("' /><br/><input type='text' id='wpass' name='wpass' placeholder='Password/Key' autocomplete='off' onchange='wformchg()' onkeyup='wformchg()' value='")); String wpass2 = wpass; wpass2.replace("'","&#39;"); client.print(wpass2); client.print(F("' /></span><span id='wb'><br/><label for='wki'>Key Index</label> <select id='wki' onchange='wformchg()'>")); for(char i=0; i<=4; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("' ")); client.print(wki==i?F("selected"):F("")); client.print(F(">")); if(i==0) client.print(F("Select")); else client.print(i,DEC); client.print(F("</option>")); } client.print(F("</select></span><br/><input id='wformsubmit' type='submit' value='Save' style='display: none;' /></form></li>"));
        
        client.print(F("<li><label>NTP sync</label><select id='b9' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(9,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off")); break;
          case 1: client.print(F("On (every hour at minute 59)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/>"));
          if(ntpSyncLast){
            client.print(F("<span id='lastsync'>Last sync as of page load time: "));
            unsigned long ntpSyncDiff = (millis()-ntpSyncLast)/1000;
            if(ntpSyncDiff<60){ client.print(ntpSyncDiff,DEC); client.print(F(" second(s) ago")); }
            else if(ntpSyncDiff<3600){ client.print(ntpSyncDiff/60,DEC); client.print(F(" minute(s) ago")); }
            else if(ntpSyncDiff<86400){ client.print(ntpSyncDiff/3600,DEC); client.print(F(" hour(s) ago")); }
            else { client.print(F(" over 24 hours ago")); } //TODO is there a display indication of this
          }
          client.print(F(" &nbsp; </span><a id='syncnow' value='' href='#' onclick='save(this); document.getElementById(\"lastsync\").remove(); return false;'>Sync&nbsp;now</a><br/><span class='explain'>Requires Wi-Fi. If using this, be sure to set your <a href='#utcoffset'>UTC offset</a> and <a href='#autodst'>auto DST</a> below.</span></li>"));
        //TODO IP address - string that separates on periods into four bytes
        
        client.print(F("<li><label>Current time</label><input type='number' id='curtodh' onchange='promptsave(\"curtod\")' onkeyup='promptsave(\"curtod\")' onblur='unpromptsave(\"curtod\"); savetod(\"curtod\")' min='0' max='23' step='1' value='")); client.print(rtcGetHour(),DEC); client.print(F("' />&nbsp;:&nbsp;<input type='number' id='curtodm' onchange='promptsave(\"curtod\")' onkeyup='promptsave(\"curtod\")' onblur='unpromptsave(\"curtod\"); savetod(\"curtod\")' min='0' max='59' step='1' value='")); client.print(rtcGetMinute(),DEC); client.print(F("' /><input type='hidden' id='curtod' /> <a id='curtodsave' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>24-hour format. Seconds will reset to 0 when saved.</span></li>"));
        
        client.print(F("<li><label>Time format</label><select id='b16' onchange='save(this)'>")); for(char i=1; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(16,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 1: client.print(F("12-hour")); break;
          case 2: client.print(F("24-hour")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>For current time display only. Alarm and setting times are always shown in 24-hour.</span></li>"));
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_DATE_FN
        client.print(F("<li><label>Current date</label><label for='curdatey'>Year&nbsp;</label><input type='number' id='curdatey' onchange='promptsave(\"curdatey\")' onkeyup='promptsave(\"curdatey\")' onblur='unpromptsave(\"curdatey\"); save(this)' min='2000' max='9999' step='1' value='")); client.print(rtcGetYear(),DEC); client.print(F("' />")); client.print(F(" <a id='curdateysave' href='#' onclick='return false' style='display: none;'>save</a>"));
        client.print(F("<br/><label for='curdatem'>Month&nbsp;</label><input type='number' id='curdatem' onchange='promptsave(\"curdatem\")' onkeyup='promptsave(\"curdatem\")' onblur='unpromptsave(\"curdatem\"); save(this)' min='1' max='12' step='1' value='")); client.print(rtcGetMonth(),DEC); client.print(F("' />")); client.print(F(" <a id='curdatemsave' href='#' onclick='return false' style='display: none;'>save</a>"));
        client.print(F("<br/><label for='curdated'>Date&nbsp;</label><input type='number' id='curdated' onchange='promptsave(\"curdated\")' onkeyup='promptsave(\"curdated\")' onblur='unpromptsave(\"curdated\"); save(this)' min='1' max='31' step='1' value='")); client.print(rtcGetDate(),DEC); client.print(F("' />")); client.print(F(" <a id='curdatedsave' href='#' onclick='return false' style='display: none;'>save</a></li>")); //TODO tip count day of year and program override to display 0 as 366 //TODO when month changes, set date max, or can you? what about 2/29? it should just do 3/1 in those cases
        
        client.print(F("<li><label>Date format</label><select id='b17' onchange='save(this)'>")); for(char i=1; i<=5; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(17,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 1: client.print(F("month/date/weekday")); break;
          case 2: client.print(F("date/month/weekday")); break;
          case 3: client.print(F("month/date/year")); break;
          case 4: client.print(F("date/month/year")); break;
          case 5: client.print(F("year/month/date")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>The weekday is displayed as a number from 0 (Sunday) to 6 (Saturday). Four-tube clocks will display only the first two values in each of these options.</span></li>"));

        //TODO Day count enabled BITMASK
            // const unsigned int FN_TIMER = 1<<0; //1
            // const unsigned int FN_DAYCOUNT = 1<<1; //2
            // const unsigned int FN_SUN = 1<<2; //4
            // const unsigned int FN_WEATHER = 1<<3; //8
        //Function preset ???????
        //TODO leap second support? Add NTP sync at top of hour also
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_DATE_COUNTER
        client.print(F("<li><label>Day counter</label><select id='b4' onchange='if(this.value==0) document.getElementById(\"daycounterdeets\").style.display=\"none\"; else document.getElementById(\"daycounterdeets\").style.display=\"inline\"; save(this)'>")); for(char i=0; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(4,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off")); break;
          case 1: client.print(F("Count days until...")); break;
          case 2: client.print(F("Count days since...")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select>"));
          client.print(F("<br/><span id='daycounterdeets' style='display: ")); if(readEEPROM(4,false)==0) client.print(F("none")); else client.print(F("inline"));
          client.print(F(";'><span></span><label for='b5'>Month&nbsp;</label><input type='number' id='b5' onchange='promptsave(\"b5\")' onkeyup='promptsave(\"b5\")' onblur='unpromptsave(\"b5\"); save(this)' min='1' max='12' step='1' value='")); client.print(readEEPROM(5,false),DEC); client.print(F("' />")); client.print(F(" <a id='b5save' href='#' onclick='return false' style='display: none;'>save</a>")); //Extra span is there to prevent "first" styling on the month label
          client.print(F("<br/><label for='b6'>Date&nbsp;</label><input type='number' id='b6' onchange='promptsave(\"b6\")' onkeyup='promptsave(\"b6\")' onblur='unpromptsave(\"b6\"); save(this)' min='1' max='31' step='1' value='")); client.print(readEEPROM(6,false),DEC); client.print(F("' />")); client.print(F(" <a id='b6save' href='#' onclick='return false' style='display: none;'>save</a><br/></span><span class='explain'>Appears after date. Repeats annually.</span></li>")); //TODO tip count day of year and program override to display 0 as 366 //TODO when month changes, set date max, or can you? what about 2/29? it should just do 3/1 in those cases
        #endif

        client.print(F("<li><label>Display date during time?</label><select id='b18' onchange='save(this)'>")); for(char i=0; i<=3; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(18,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Never")); break;
          case 1: client.print(F("Date instead of seconds")); break;
          case 2: client.print(F("Full date at :30 seconds (instant)")); break;
          case 3: client.print(F("Full date at :30 seconds (scrolling)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif //date section
          
        client.print(F("<li><label>Leading zeros</label><select id='b19' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(19,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("No (1:23)")); break;
          case 1: client.print(F("Yes (01:23)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        
        #if SHOW_IRRELEVANT_OPTIONS || defined(DISPLAY_NIXIE)
        client.print(F("<li><label>Digit fade</label><input type='number' id='b20' onchange='promptsave(\"b20\")' onkeyup='promptsave(\"b20\")' onblur='unpromptsave(\"b20\"); save(this)' min='0' max='20' step='1' value='")); client.print(readEEPROM(20,false),DEC); client.print(F("' />")); client.print(F(" <a id='b20save' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>Nixie tube digit fade effect, in hundredths of a second (up to 20)</span></li>"));
        #endif //nixie
          
        client.print(F("<li><label>Auto DST</label><a name='autodst' href='#'></a><select id='b22' onchange='save(this)'>")); for(char i=0; i<=6; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(22,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off")); break;
          case 1: client.print(F("March–November (US/CA)")); break;
          case 2: client.print(F("March–October (UK/EU)")); break;
          case 3: client.print(F("April–October (MX)")); break;
          case 4: client.print(F("September–April (NZ)")); break;
          case 5: client.print(F("October–April (AU)")); break;
          case 6: client.print(F("October–February (BZ)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>Automatically sets clock forward/backward at 2am on the relevant Sunday (see instructions for details). If you observe Daylight Saving Time but your locale's rules are not represented here, leave this set to Off and set the clock forward manually (and add an hour to the <a href='#utcoffset'>UTC offset</a> if using sunrise/sunset).</span></li>")); //TODO instructions

          //TODO call everything backlight
        #if SHOW_IRRELEVANT_OPTIONS || LED_PIN>=0
        client.print(F("<li><label>Backlight behavior</label><a name='backlight' href='#'></a><select id='b26' onchange='save(this)'>")); for(char i=0; i<=4; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(26,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Always off")); break;
          case 1: client.print(F("Always on")); break;
          case 2: client.print(F("On until night/away shutoff")); break;
          case 3: client.print(F("Off until alarm/timer sounds")); break;
          case 4: client.print(F("Off until switched relay")); break; //TODO skip
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif //led pin

          //TODO nixie only TODO option for never
        #if SHOW_IRRELEVANT_OPTIONS || defined(DISPLAY_NIXIE)
        client.print(F("<li><label>Anti-cathode poisoning</label><a name='antipoison' href='#'></a><select id='b46' onchange='save(this)'>")); for(char i=0; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(46,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Once a day")); break;
          case 1: client.print(F("Every hour")); break;
          case 2: client.print(F("Every minute")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>Briefly cycles all digits to prevent <a href='http://www.tube-tester.com/sites/nixie/different/cathode%20poisoning/cathode-poisoning.htm' target='_blank'>cathode poisoning</a>. Will not trigger during night/away shutoff. Daily option happens at midnight or, if enabled, when night shutoff starts.</span></li>"));
        #endif //nixie

        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_TEMP_FN //TODO good for weather also
        client.print(F("<li><label>Temperature scale</label><select id='b45' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(46,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("°C")); break;
          case 1: client.print(F("°F")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif //temp
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_ALARM_FN
        client.print(F("<li><h3>Alarm</h3></li>"));
        
        client.print(F("<li><label>Alarm is&hellip;</label><select id='alm' onchange='save(this)'>")); for(char i=0; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if((i<2 && readEEPROM(2,false)==i) || alarmSkip) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off (0)")); break;
          case 1: client.print(F("On (1)")); break;
          case 2: client.print(F("On, but skip next (01)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        
        client.print(F("<li><label>Alarm time</label><input type='number' id='almtodh' onchange='promptsave(\"almtod\")' onkeyup='promptsave(\"almtod\")' onblur='unpromptsave(\"almtod\"); savetod(\"almtod\")' min='0' max='23' step='1' value='")); client.print(readEEPROM(0,true)/60,DEC); client.print(F("' />&nbsp;:&nbsp;<input type='number' id='almtodm' onchange='promptsave(\"almtod\")' onkeyup='promptsave(\"almtod\")' onblur='unpromptsave(\"almtod\"); savetod(\"almtod\")' min='0' max='59' step='1' value='")); client.print(readEEPROM(0,true)%60,DEC); client.print(F("' /><input type='hidden' id='almtod' /> <a id='almtodsave' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>24-hour format.</span></li>"));
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_ALARM_AUTOSKIP
        client.print(F("<li><label>Auto-skip</label><select id='b23' onchange='save(this)'>")); for(char i=0; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(23,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Never (alarm every day)")); break;
          case 1: client.print(F("Weekends")); break;
          case 2: client.print(F("Work week")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>If using this, be sure to set <a href='#workweek'>work week</a> below.</span></li>"));
        #endif

        #if SHOW_IRRELEVANT_OPTIONS || (RELAY_PIN>=0 && PIEZO_PIN>=0)
        client.print(F("<li><label>Signal</label><select id='b42' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(42,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Beeper")); break;
          case 1: client.print(F("Relay")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select>"));
          #if RELAY_MODE
            client.print(F("<br><span class='explain'>Relay will automatically switch off after ")); client.print(SWITCH_DUR/60,DEC); client.print(F(" minutes.</span>"));
          #endif
          client.print(F("</li>"));
        #endif
        
        #if SHOW_IRRELEVANT_OPTIONS || (PIEZO_PIN>=0)
        client.print(F("<li><label>Pitch</label><select id='b39' onchange='save(this)'>")); for(char i=49; i<=88; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(39,false)==i) client.print(F(" selected")); client.print(F(">")); switch((i-40)%12){
          case 0: client.print(F("C")); break;
          case 1: client.print(F("D&#9837;")); break;
          case 2: client.print(F("D")); break;
          case 3: client.print(F("E&#9837;")); break;
          case 4: client.print(F("E")); break;
          case 5: client.print(F("F")); break;
          case 6: client.print(F("G&#9837;")); break;
          case 7: client.print(F("G")); break;
          case 8: client.print(F("A&#9837;")); break;
          case 9: client.print(F("A")); break;
          case 10: client.print(F("B&#9837;")); break;
          case 11: client.print(F("B")); break;
          default: break; } client.print(((i-40)/12)+4,DEC); client.print(F("</option>")); } client.print(F("</select></li>"));
          
        client.print(F("<li><label>Pattern</label><select id='b47' onchange='save(this)'>")); for(char i=0; i<=5; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(47,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Long")); break;
          case 1: client.print(F("Short")); break;
          case 2: client.print(F("Double")); break;
          case 3: client.print(F("Triple")); break;
          case 4: client.print(F("Quad")); break;
          case 5: client.print(F("Cuckoo")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif
        
        client.print(F("<li><label>Snooze</label><input type='number' id='b24' onchange='promptsave(\"b24\")' onkeyup='promptsave(\"b24\")' onblur='unpromptsave(\"b24\"); save(this)' min='0' max='60' step='1' value='")); client.print(readEEPROM(24,false),DEC); client.print(F("' />")); client.print(F(" <a id='b24save' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>In minutes. Zero disables snooze.</span></li>"));
        
        #if SHOW_IRRELEVANT_OPTIONS || (PIEZO_PIN>=0 && RELAY_MODE>0 && ENABLE_ALARM_FIBONACCI)
        client.print(F("<li><label>Fibonacci mode</label><select id='b50' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(50,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off")); break;
          case 1: client.print(F("On")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br><span class='explain'>To wake you more gradually, the alarm will start about 27 minutes early, by beeping at increasingly shorter intervals per the Fibonacci sequence (610 seconds, then 337, then 233...). In this mode, snooze does not take effect; any button press will silence the alarm for the day, even if the set alarm time hasn’t been reached yet."));
          #if (RELAY_PIN>=0 && PIEZO_PIN>=0 && RELAY_MODE>0) //when switched relay is possible alarm signal
            client.print(F(" Has no effect when alarm is set to use relay."));
          #endif
          client.print(F("</span></li>"));
        #endif
          
        #endif //alarm section
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_TIMER_FN
        client.print(F("<li><h3>Chrono/Timer</h3></li>"));
        
        client.print(F("<li><label>Timer runout</label><select id='runout' onchange='save(this)'>")); for(char i=0; i<=3; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(((timerState>>2)&3)==i) client.print(F(" selected")); client.print(F(">")); switch(i){ //00 stop, 01 repeat, 10 chrono, 11 chrono short signal
          case 0: client.print(F("Stop, long signal")); break;
          case 1: client.print(F("Repeat, short signal")); break;
          case 2: client.print(F("Start chrono, long signal")); break;
          case 3: client.print(F("Start chrono, short signal")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br><span class='explain'>What the timer will do when it runs out. This can be set directly on the clock as well: while the timer is running, Down will cycle through these options (1-4 beeps respectively). The repeat option makes a great interval timer!</span></li>")); //TODO switch etc
        
        #if SHOW_IRRELEVANT_OPTIONS || (RELAY_PIN>=0 && PIEZO_PIN>=0)
        client.print(F("<li><label>Signal</label><select id='b43' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(43,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Beeper")); break;
          case 1: client.print(F("Relay")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select>"));
          #if RELAY_MODE
            client.print(F("<br><span class='explain'>Relay will switch on while timer is running, like a <a href='https://en.wikipedia.org/wiki/Time_switch' target='_blank'>sleep timer</a>.</span>"));
          #endif
          client.print(F("</li>"));
        #endif
        
        #if SHOW_IRRELEVANT_OPTIONS || (PIEZO_PIN>=0)
        client.print(F("<li><label>Pitch</label><select id='b40' onchange='save(this)'>")); for(char i=49; i<=88; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(40,false)==i) client.print(F(" selected")); client.print(F(">")); switch((i-40)%12){
          case 0: client.print(F("C")); break;
          case 1: client.print(F("D&#9837;")); break;
          case 2: client.print(F("D")); break;
          case 3: client.print(F("E&#9837;")); break;
          case 4: client.print(F("E")); break;
          case 5: client.print(F("F")); break;
          case 6: client.print(F("G&#9837;")); break;
          case 7: client.print(F("G")); break;
          case 8: client.print(F("A&#9837;")); break;
          case 9: client.print(F("A")); break;
          case 10: client.print(F("B&#9837;")); break;
          case 11: client.print(F("B")); break;
          default: break; } client.print(((i-40)/12)+4,DEC); client.print(F("</option>")); } client.print(F("</select></li>"));
        
        client.print(F("<li><label>Pattern</label><select id='b48' onchange='save(this)'>")); for(char i=0; i<=5; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(48,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Long")); break;
          case 1: client.print(F("Short")); break;
          case 2: client.print(F("Double")); break;
          case 3: client.print(F("Triple")); break;
          case 4: client.print(F("Quad")); break;
          case 5: client.print(F("Cuckoo")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif
          
        #endif //timer/chrono section
        
        #if SHOW_IRRELEVANT_OPTIONS || (ENABLE_TIME_CHIME && PIEZO_PIN>=0 && RELAY_MODE>0)
        client.print(F("<li><h3>Chime</h3></li>"));
        
        client.print(F("<li><label>Chime</label><select id='b21' onchange='save(this)'>")); for(char i=0; i<=4; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(21,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off")); break;
          case 1: client.print(F("Single pulse")); break;
          case 2: client.print(F("Six pips")); break;
          case 3: client.print(F("Strike the hour")); break;
          case 4: client.print(F("Ship's bell")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>Will not sound during night/away shutoff (except when off starts at top of hour). <a href='https://en.wikipedia.org/wiki/Greenwich_Time_Signal' target='_blank'>Six pips refers to this.</a></span></li>"));
        
        #if SHOW_IRRELEVANT_OPTIONS || (RELAY_PIN>=0 && PIEZO_PIN>=0 && RELAY_MODE>0)
        client.print(F("<li><label>Signal</label><select id='b44' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(44,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Beeper")); break;
          case 1: client.print(F("Relay")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif
        
        #if SHOW_IRRELEVANT_OPTIONS || (PIEZO_PIN>=0)  
        client.print(F("<li><label>Pitch</label><select id='b41' onchange='save(this)'>")); for(char i=49; i<=88; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(41,false)==i) client.print(F(" selected")); client.print(F(">")); switch((i-40)%12){
          case 0: client.print(F("C")); break;
          case 1: client.print(F("D&#9837;")); break;
          case 2: client.print(F("D")); break;
          case 3: client.print(F("E&#9837;")); break;
          case 4: client.print(F("E")); break;
          case 5: client.print(F("F")); break;
          case 6: client.print(F("G&#9837;")); break;
          case 7: client.print(F("G")); break;
          case 8: client.print(F("A&#9837;")); break;
          case 9: client.print(F("A")); break;
          case 10: client.print(F("B&#9837;")); break;
          case 11: client.print(F("B")); break;
          default: break; } client.print(((i-40)/12)+4,DEC); client.print(F("</option>")); } client.print(F("</select></li>"));
          
        client.print(F("<li><label>Pattern</label><select id='b49' onchange='save(this)'>")); for(char i=0; i<=5; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(49,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Long")); break;
          case 1: client.print(F("Short")); break;
          case 2: client.print(F("Double")); break;
          case 3: client.print(F("Triple")); break;
          case 4: client.print(F("Quad")); break;
          case 5: client.print(F("Cuckoo")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif
          
        #endif //chime section
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_SHUTOFF_NIGHT || ENABLE_SHUTOFF_AWAY
        client.print(F("<li><h3>Shutoff</h3></li>"));
        #endif
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_SHUTOFF_NIGHT
        client.print(F("<li><label>Night shutoff</label><select id='b27' onchange='save(this)'>")); for(char i=0; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(27,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("None")); break;
          case 1: client.print(F("Dim")); break;
          case 2: client.print(F("Shut off")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>To save display life and/or preserve your sleep, dim or shut off display nightly when you're not around or sleeping. When off, you can press Select to illuminate the display briefly.</span></li>"));
        
        client.print(F("<li><label>Night start</label><input type='number' id='nighttodh' onchange='promptsave(\"nighttod\")' onkeyup='promptsave(\"nighttod\")' onblur='unpromptsave(\"nighttod\"); savetod(\"nighttod\")' min='0' max='23' step='1' value='")); client.print(readEEPROM(28,true)/60,DEC); client.print(F("' />&nbsp;:&nbsp;<input type='number' id='nighttodm' onchange='promptsave(\"nighttod\")' onkeyup='promptsave(\"nighttod\")' onblur='unpromptsave(\"nighttod\"); savetod(\"nighttod\")' min='0' max='59' step='1' value='")); client.print(readEEPROM(28,true)%60,DEC); client.print(F("' /><input type='hidden' id='nighttod' /> <a id='nighttodsave' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>24-hour format.</span></li>"));
        
        client.print(F("<li><label>Night end</label><input type='number' id='morntodh' onchange='promptsave(\"morntod\")' onkeyup='promptsave(\"morntod\")' onblur='unpromptsave(\"morntod\"); savetod(\"morntod\")' min='0' max='23' step='1' value='")); client.print(readEEPROM(30,true)/60,DEC); client.print(F("' />&nbsp;:&nbsp;<input type='number' id='morntodm' onchange='promptsave(\"morntod\")' onkeyup='promptsave(\"morntod\")' onblur='unpromptsave(\"morntod\"); savetod(\"morntod\")' min='0' max='59' step='1' value='")); client.print(readEEPROM(30,true)%60,DEC); client.print(F("' /><input type='hidden' id='morntod' /> <a id='morntodsave' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>24-hour format. Set to 0:00 to use the alarm time.</span></li>"));
        #endif //night shutoff
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_SHUTOFF_AWAY
        client.print(F("<li><label>Away shutoff</label><select id='b32' onchange='save(this)'>")); for(char i=0; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(32,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("None")); break;
          case 1: client.print(F("Weekends (clock at work)")); break;
          case 2: client.print(F("Workday (clock at home)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>To further save display life, shut off display during daytime hours when you're not around. This feature is designed to accommodate your weekly work schedule.</span></li>"));
        #endif
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_SHUTOFF_AWAY || (ENABLE_ALARM_FN && ENABLE_ALARM_AUTOSKIP)
          #if !SHOW_IRRELEVANT_OPTIONS && !ENABLE_SHUTOFF_AWAY //Alternative header if only workweek is needed
          client.print(F("<li><h3>Workweek</h3></li>"));
          #endif
        client.print(F("<li><label>First day of workweek</label><a name='workweek' href='#'></a><select id='b33' onchange='save(this)'>")); for(char i=0; i<=6; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(33,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Sunday")); break;
          case 1: client.print(F("Monday")); break;
          case 2: client.print(F("Tuesday")); break;
          case 3: client.print(F("Wednesday")); break;
          case 4: client.print(F("Thursday")); break;
          case 5: client.print(F("Friday")); break;
          case 6: client.print(F("Saturday")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        
        client.print(F("<li><label>Last day of workweek</label><select id='b34' onchange='save(this)'>")); for(char i=0; i<=6; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(34,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Sunday")); break;
          case 1: client.print(F("Monday")); break;
          case 2: client.print(F("Tuesday")); break;
          case 3: client.print(F("Wednesday")); break;
          case 4: client.print(F("Thursday")); break;
          case 5: client.print(F("Friday")); break;
          case 6: client.print(F("Saturday")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        #endif
        
        #if SHOW_IRRELEVANT_OPTIONS || ENABLE_SHUTOFF_AWAY
        client.print(F("<li><label>Workday start</label><input type='number' id='worktodh' onchange='promptsave(\"worktod\")' onkeyup='promptsave(\"worktod\")' onblur='unpromptsave(\"worktod\"); savetod(\"worktod\")' min='0' max='23' step='1' value='")); client.print(readEEPROM(35,true)/60,DEC); client.print(F("' />&nbsp;:&nbsp;<input type='number' id='worktodm' onchange='promptsave(\"worktod\")' onkeyup='promptsave(\"worktod\")' onblur='unpromptsave(\"worktod\"); savetod(\"worktod\")' min='0' max='59' step='1' value='")); client.print(readEEPROM(35,true)%60,DEC); client.print(F("' /><input type='hidden' id='worktod' /> <a id='worktodsave' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>24-hour format.</span></li>"));
      
        client.print(F("<li><label>Workday end</label><input type='number' id='hometodh' onchange='promptsave(\"hometod\")' onkeyup='promptsave(\"hometod\")' onblur='unpromptsave(\"hometod\"); savetod(\"hometod\")' min='0' max='23' step='1' value='")); client.print(readEEPROM(37,true)/60,DEC); client.print(F("' />&nbsp;:&nbsp;<input type='number' id='hometodm' onchange='promptsave(\"hometod\")' onkeyup='promptsave(\"hometod\")' onblur='unpromptsave(\"hometod\"); savetod(\"hometod\")' min='0' max='59' step='1' value='")); client.print(readEEPROM(37,true)%60,DEC); client.print(F("' /><input type='hidden' id='hometod' /> <a id='hometodsave' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>24-hour format. Set to 0:00 to use the alarm time.</span></li>"));  
        #endif //away shutoff
        
        client.print(F("<li><h3>Geography</h3></li>"));
        
        #if SHOW_IRRELEVANT_OPTIONS || (ENABLE_DATE_FN && ENABLE_DATE_RISESET)
        client.print(F("<li><p><a href='https://support.google.com/maps/answer/18539?co=GENIE.Platform%3DDesktop&hl=en' target='_blank'>How to find your latitude and longitude</a></p></li>"));

        client.print(F("<li><label>Latitude</label><input type='number' id='i10raw' onchange='promptsave(\"i10\")' onkeyup='promptsave(\"i10\")' onblur='unpromptsave(\"i10\"); savecoord(\"i10\")' min='-90' max='90' step='0.1' value='")); client.print(readEEPROM(10,true)/10,DEC); client.print(F(".")); client.print(abs(readEEPROM(10,true))%10,DEC); client.print(F("' /><input type='hidden' id='i10' /> <a id='i10save' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>Your latitude, to the nearest tenth of a degree. Negative values are south.</span></li>"));

        client.print(F("<li><label>Longitude</label><input type='number' id='i12raw' onchange='promptsave(\"i12\")' onkeyup='promptsave(\"i12\")' onblur='unpromptsave(\"i12\"); savecoord(\"i12\")' min='-180' max='180' step='0.1' value='")); client.print(readEEPROM(12,true)/10,DEC); client.print(F(".")); client.print(abs(readEEPROM(12,true))%10,DEC); client.print(F("' /><input type='hidden' id='i12' /> <a id='i12save' href='#' onclick='return false' style='display: none;'>save</a><br/><span class='explain'>Your longitude, to the nearest tenth of a degree. Negative values are west.</span></li>"));
        #endif //date rise set
        
        //Wi-Fi, NTP, and UTC offset are always relevant given network
        client.print(F("<li><label>UTC offset</label><a name='utcoffset' href='#'></a><select id='b14' onchange='save(this)'>")); for(char i=52; i<=156; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(14,false)==i) client.print(F(" selected")); client.print(F(">"));
        char offseth = abs(i-100)/4;
        char offsetm = (abs(i-100)%4)*15;
        if(i<100) client.print(F("–")); else client.print(F("+"));
        client.print(offseth,DEC);
        client.print(F(":"));
        if(offsetm<10) client.print(F("0"));
        client.print(offsetm,DEC);
        client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>Your time zone's offset from UTC (non-DST). If you observe DST but set the clock manually rather than using the <a href='#autodst'>auto DST</a> feature, you must add an hour to the UTC offset during DST, or the sunrise/sunset times will be an hour early.</span></li>"));
        
        //After replacing the below from formdev.php, replace " with \"
        client.print(F("</ul></div><script type='text/javascript'>function e(id){ return document.getElementById(id); } function promptsave(ctrl){ document.getElementById(ctrl+\"save\").style.display=\"inline\"; } function unpromptsave(ctrl){ document.getElementById(ctrl+\"save\").style.display=\"none\"; } function savecoord(ctrlset){ ctrl = document.getElementById(ctrlset); if(ctrl.disabled) return; ctrl.value = parseInt(parseFloat(document.getElementById(ctrlset+\"raw\").value)*10); save(ctrl); } function savetod(ctrlset){ ctrl = document.getElementById(ctrlset); if(ctrl.disabled) return; ctrl.value = (parseInt(document.getElementById(ctrlset+\"h\").value)*60) + parseInt(document.getElementById(ctrlset+\"m\").value); save(ctrl); } function save(ctrl){ if(ctrl.disabled) return; ctrl.disabled = true; let ind = ctrl.nextSibling; if(ind && ind.tagName==='SPAN') ind.parentNode.removeChild(ind); ind = document.createElement('span'); ind.innerHTML = '&nbsp;<span class=\"saving\">Saving&hellip;</span>'; ctrl.parentNode.insertBefore(ind,ctrl.nextSibling); let xhr = new XMLHttpRequest(); xhr.onreadystatechange = function(){ if(xhr.readyState==4){ ctrl.disabled = false; console.log(xhr); if(xhr.status==200 && xhr.responseText=='ok'){ if(ctrl.id=='wform'){ e('content').innerHTML = '<p class=\"ok\">Wi-Fi changes applied.</p><p>' + (e('wssid').value? 'Now attempting to connect to <strong>'+htmlEntities(e('wssid').value)+'</strong>.</p><p>If successful, the clock will display its IP address. To access this settings page again, connect to <strong>'+htmlEntities(e('wssid').value)+'</strong> and visit that IP address. (If you miss it, hold Select for 5 seconds to see it again.)</p><p>If not successful, the clock will display <strong>7777</strong>. ': '') + 'To access this settings page again, (re)connect to Wi-Fi network <strong>Clock</strong> and visit <a href=\"http://7.7.7.7\">7.7.7.7</a>.</p>'; clearTimeout(timer); } else { ind.innerHTML = '&nbsp;<span class=\"ok\">OK!</span>'; setTimeout(function(){ if(ind.parentNode) ind.parentNode.removeChild(ind); },1500); } } else ind.innerHTML = '&nbsp;<span class=\"error\">'+(xhr.responseText?xhr.responseText:'Error')+'</span>'; timer = setTimeout(timedOut, ")); client.print(ADMIN_TIMEOUT,DEC); client.print(F("); } }; clearTimeout(timer); xhr.open('POST', './', true); xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded'); if(ctrl.id=='wform'){ switch(e('wtype').value){ case '': e('wssid').value = ''; e('wpass').value = ''; case 'wpa': e('wki').value = '0'; case 'wep': default: break; } xhr.send('wssid='+e('wssid').value+'&wpass='+e('wpass').value+'&wki='+e('wki').value); } else { xhr.send(ctrl.id+'='+ctrl.value); } } function wformchg(initial){ if(initial) e('wtype').value = (e('wssid').value? (e('wki').value!=0? 'wep': 'wpa'): ''); e('wa').style.display = (e('wtype').value==''?'none':'inline'); e('wb').style.display = (e('wtype').value=='wep'?'inline':'none'); if(!initial) e('wformsubmit').style.display = 'inline'; } function timedOut(){ e('content').innerHTML = 'Clock settings page has timed out. Please hold Select for 5 seconds to reactivate it, then <a href=\"#\" onclick=\"location.reload(); return false;\">refresh</a>.'; } function htmlEntities(str){ return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/\"/g, '&quot;'); } wformchg(true); let timer = setTimeout(timedOut, ")); client.print(ADMIN_TIMEOUT,DEC); client.print(F("); document.getElementById('loading').remove(); document.getElementById('content').style.display = 'block';</script></body></html>"));
        //client.print(F(""));
      } //end get
      else { //requestType==2 - handle what was POSTed
        String clientReturn = "ok"; //To return an error, set it here. An empty response generates "Error".
        //client.print(currentLine);
        //syncfreq=hr
        //syncfreq=min
        if(currentLine.startsWith(F("wssid="))){ //wifi change
          //e.g. wssid=Network Name&wpass=qwertyuiop&wki=1
          //TODO since the values are not html-entitied (due to the difficulty of de-entiting here), this will fail if the ssid contains "&wssid=" or pass contains "&wpass="
          int startPos = 6;
          int endPos = currentLine.indexOf(F("&wpass="),startPos);
          wssid = currentLine.substring(startPos,endPos);
          startPos = endPos+7;
          endPos = currentLine.indexOf(F("&wki="),startPos);
          wpass = currentLine.substring(startPos,endPos);
          startPos = endPos+5;
          wki = currentLine.substring(startPos).toInt();
            // client.print(F(" wssid="));
            // client.print(wssid);
            // client.print(F(" wpass="));
            // client.print(wpass);
            // client.print(F(" wki="));
            // client.print(wki);
            // client.println();
          requestType = 3; //triggers an admin restart after the client is closed, below
        } else if(currentLine.startsWith(F("syncnow"))){
          cueNTP();
        } else if(currentLine.startsWith(F("curtod"))){
          int curtod = currentLine.substring(7).toInt();
          rtcSetTime(curtod/60,curtod%60,0);
          goToFn(fnIsTime);
        } else if(currentLine.startsWith(F("curdatey"))){
          rtcSetDate(currentLine.substring(9).toInt(), rtcGetMonth(), rtcGetDate(), dayOfWeek(currentLine.substring(9).toInt(), rtcGetMonth(), rtcGetDate())); //TODO what about month exceed
          goToFn(fnIsDate); fnPg = 254;
        } else if(currentLine.startsWith(F("curdatem"))){
          rtcSetDate(rtcGetYear(), currentLine.substring(9).toInt(), rtcGetDate(), dayOfWeek(rtcGetYear(), currentLine.substring(9).toInt(), rtcGetDate())); //TODO what about month exceed
          goToFn(fnIsDate); fnPg = 254;
        } else if(currentLine.startsWith(F("curdated"))){
          rtcSetDate(rtcGetYear(), rtcGetMonth(), currentLine.substring(9).toInt(), dayOfWeek(rtcGetYear(), rtcGetMonth(), currentLine.substring(9).toInt())); //TODO what about month exceed
          goToFn(fnIsDate); fnPg = 254;
        } else if(currentLine.startsWith(F("almtod"))){
          writeEEPROM(0,currentLine.substring(7).toInt(),true);
          goToFn(fnIsAlarm);
        } else if(currentLine.startsWith(F("alm"))){ //two settings (alarm on, alarm skip) with one control
          char alm = currentLine.substring(4).toInt();
          if(alm<2){ writeEEPROM(2,alm,false); alarmSkip = false; }
          else     { writeEEPROM(2,1,false);   alarmSkip = true; }
          //TODO beeps and set fn
          goToFn(fnIsAlarm);
        } else if(currentLine.startsWith(F("runout"))){
          char runout = currentLine.substring(7).toInt();
          if(runout/2) timerState |= (1<<3); else timerState &= ~(1<<3); //chrono bit
          if(runout%2) timerState |= (1<<2); else timerState &= ~(1<<2); //restart bit
        } else if(currentLine.startsWith(F("nighttod"))){
          writeEEPROM(28,currentLine.substring(9).toInt(),true);
        } else if(currentLine.startsWith(F("morntod"))){
          writeEEPROM(30,currentLine.substring(8).toInt(),true);
        } else if(currentLine.startsWith(F("worktod"))){
          writeEEPROM(35,currentLine.substring(8).toInt(),true);
        } else if(currentLine.startsWith(F("hometod"))){
          writeEEPROM(37,currentLine.substring(8).toInt(),true);
        } else {
          //standard eeprom saves by type/loc
          bool isInt = currentLine.startsWith(F("i")); //or b for byte
          int eqPos = currentLine.indexOf(F("="));
          int key = currentLine.substring(1,eqPos).toInt();
          int val = currentLine.substring(eqPos+1).toInt();
          writeEEPROM(key,val,isInt);
          //do special stuff for some of them
          switch(key){
            case 4: case 5: case 6: //day counter
              //in lieu of actually switching to fnIsDate, so that only this value is seen - compare to ino
              if(readEEPROM(4,false)) tempValDispQueue[0] = dateComp(rtcGetYear(),rtcGetMonth(),rtcGetDate(), readEEPROM(5,false),readEEPROM(6,false),readEEPROM(4,false)-1);
              findFnAndPageNumbers(); //to include or exclude the day counter from the calendar function
              break;
            case 14: //utc offset
              cueNTP(); break;
            case 17: //date format
              goToFn(fnIsDate); fnPg = 254; break;
            case 22: //auto dst
              isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),rtcGetHour(),true); break;
            case 39: //alarm pitch //TODO beep
              goToFn(fnIsAlarm); break;
            case 40: //timer pitch //TODO beep
              goToFn(fnIsTimer); break;
            case 41: //strike pitch //TODO beep
              goToFn(fnIsTime); break;
            case 47: //alarm pattern //TODO beep
              goToFn(fnIsAlarm); break;
            case 48: //timer pattern //TODO beep
              goToFn(fnIsTimer); break;
            case 49: //strike pattern //TODO beep
              goToFn(fnIsTime); break;
            default: break;
          }
        }
        updateDisplay();
        client.print(clientReturn);
      } //end post
    } //end if requestType
    
    client.stop();
    Serial.println("");
    Serial.println("client disconnected");
    delay(500); //for client to get the message TODO why is this necessary
    
    if(requestType==3) { //wifi was changed - restart the admin
      networkStartWiFi(); //try to connect to wifi with new settings
      networkStartAdmin(); //will set up AP if wifi isn't connected
    }
  }
}

void initNetwork(){
  //Check status of wifi module up front
  if(WiFi.status()==WL_NO_MODULE){ Serial.println(F("Communication with WiFi module failed!")); while(true); }
  else if(WiFi.firmwareVersion()<WIFI_FIRMWARE_LATEST_VERSION) Serial.println(F("Please upgrade the firmware"));
  networkStartWiFi();  
}
void cycleNetwork(){
  checkClients();
  checkNTP();
  checkForWiFiStatusChange();
}

#endif

#endif
#endif