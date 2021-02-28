#ifdef NETWORK //only compile when requested (when included in main file)
#ifndef NETWORK_SRC //include once only
#define NETWORK_SRC

#ifndef __AVR__ //TODO better sensor
//do stuff for wifinina
  
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

const unsigned long ADMIN_TIMEOUT = 120000; //two minutes
const unsigned long NTPOK_THRESHOLD = 3600000; //if no sync within 60 minutes, the time is considered stale

//Declare a few functions so I can put them out of order
//Placed here so I can avoid making header files for the moment
void startNTP();

int statusLast;
void checkForWiFiStatusChange(){
  if(WiFi.status()!=statusLast){
    Serial.print(millis()); Serial.print(F(" WiFi status has changed to "));
    statusLast = WiFi.status();
    switch(statusLast){
      case WL_IDLE_STATUS: Serial.print(F("WL_IDLE_STATUS")); break;
      case WL_NO_SSID_AVAIL: Serial.print(F("WL_NO_SSID_AVAIL")); break;
      case WL_SCAN_COMPLETED: Serial.print(F("WL_SCAN_COMPLETED")); break;
      case WL_CONNECTED: Serial.print(F("WL_CONNECTED")); break;
      case WL_CONNECT_FAILED: Serial.print(F("WL_CONNECT_FAILED")); break;
      case WL_CONNECTION_LOST: Serial.print(F("WL_CONNECTION_LOST")); break;
      case WL_DISCONNECTED: Serial.print(F("WL_DISCONNECTED")); break;
      case WL_AP_LISTENING: Serial.print(F("WL_AP_LISTENING")); break;
      case WL_AP_CONNECTED: Serial.print(F("WL_AP_CONNECTED")); break;
      default: break;
    }
    Serial.print(F(" (")); Serial.print(WiFi.status()); Serial.println(F(")"));
  }
}

void networkStartWiFi(){
  WiFi.end(); //if AP is going, stop it
  if(wssid==F("")) return; //don't try to connect if there's no creds
  checkForWiFiStatusChange(); //just for serial logging
  //rtcDisplayTime(false); //display time without seconds //TODO replace this with what
  blankDisplay(0,5,false); //I'm guessing if it hangs, nixies won't be able to display anyway
  Serial.println(); Serial.print(millis()); Serial.print(F(" Attempting to connect to SSID: ")); Serial.println(wssid);
  if(wki) WiFi.begin(wssid.c_str(), wki, wpass.c_str()); //WEP - hangs while connecting
  else WiFi.begin(wssid.c_str(), wpass.c_str()); //WPA - hangs while connecting
  if(WiFi.status()==WL_CONNECTED){ //did it work?
    Serial.print(millis()); Serial.println(F(" Connected!"));
    Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
    Serial.print(F("Signal strength (RSSI):")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
    Serial.print(F("Access the admin page by browsing to http://")); Serial.println(WiFi.localIP());
    server.begin(); Udp.begin(localPort); startNTP();
  }
  else Serial.println(F(" Wasn't able to connect."));
  updateDisplay();
  checkForWiFiStatusChange(); //just for serial logging
} //end fn startWiFi

void networkStartAP(){
  WiFi.end(); //if wifi is going, stop it
  checkForWiFiStatusChange(); //just for serial logging
  Serial.println(); Serial.print(millis()); Serial.println(F(" Creating access point"));
  if(WiFi.beginAP("Clock")==WL_AP_LISTENING){ //Change "beginAP" if you want to create an WEP network
    Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
    //by default the local IP address of will be 192.168.4.1 - override with WiFi.config(IPAddress(10, 0, 0, 1));
    WiFi.config(IPAddress(7,7,7,7));
    Serial.print(F("Access the admin page by browsing to http://")); Serial.println(WiFi.localIP());
    //server.begin() was formerly here
    server.begin();
  }
  else Serial.println(F(" Wasn't able to create access point."));
  checkForWiFiStatusChange(); //just for serial logging
} //end fn startAP

void networkDisconnectWiFi(){
  //Serial.println(F("Disconnecting WiFi - will try to connect at next NTP sync time"));
  WiFi.end();
}

unsigned int ntpStartLast = -3000;
bool ntpGoing = 0;
unsigned int ntpSyncLast = 0;
void startNTP(){ //Called at intervals to check for ntp time
  if(wssid==F("")) return; //don't try to connect if there's no creds
  if(WiFi.status()!=WL_CONNECTED) networkStartWiFi(); //in case it dropped
  if(WiFi.status()!=WL_CONNECTED) return;
  if(ntpGoing && millis()-ntpStartLast < 3000) return; //if a previous request is going, do not start another until at least 3sec later
  ntpGoing = 1;
  ntpStartLast = millis();
  Udp.flush(); //in case of old data
  //Udp.stop() was formerly here
  Serial.println(); Serial.print(millis()); Serial.println(F(" Sending UDP packet to NTP server."));
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
} //end fn startNTP

bool TESTNTPfail = 0;
void checkNTP(){ //Called on every cycle to see if there is an ntp response to handle
  if(!ntpGoing) return;
  if(!Udp.parsePacket()) return;
  // We've received a packet, read the data from it
  ntpSyncLast = millis();
  unsigned int requestTime = ntpSyncLast-ntpStartLast;
  Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
  if(TESTNTPfail) { Udp.flush(); return; Serial.println(F("NTP came back but discarding")); } //you didn't see anything...
  
  //https://forum.arduino.cc/index.php?topic=526792.0
  unsigned long ntpTime = (packetBuffer[40] << 24) | (packetBuffer[41] << 16) | (packetBuffer[42] << 8) | packetBuffer[43];
  unsigned long ntpFrac = (packetBuffer[44] << 24) | (packetBuffer[45] << 16) | (packetBuffer[46] << 8) | packetBuffer[47];
  unsigned int  ntpMils = (int32_t)(((float)ntpFrac / UINT32_MAX) * 1000);
  
  //Convert unix timestamp to UTC date/time
  unsigned long ntpPart = ntpTime;
  int y = 1970;
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
  int utcohm = (readEEPROM(14,false)-100); //utc offset in mins from midnight
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
  //TODO how to do subsecond set - do we push the clock forward and delay that amount of time? would involve checking for hour rollover. Also take into account requestTime/2
  rtcSetTime(hm/60,hm%60,s);
  millisAtLastCheck = 0; //see ms()
  calcSun();
  
  Serial.print(millis());
  Serial.print(F(" Received UDP packet from NTP server and set RTC to "));
  Serial.print(rtcGetYear(),DEC); Serial.print(F("-"));
  if(rtcGetMonth()<10) Serial.print(F("0")); Serial.print(rtcGetMonth(),DEC); Serial.print(F("-"));
  if(rtcGetDate()<10) Serial.print(F("0")); Serial.print(rtcGetDate(),DEC); Serial.print(F(" "));
  if(rtcGetHour()<10) Serial.print(F("0")); Serial.print(rtcGetHour(),DEC); Serial.print(F(":"));
  if(rtcGetMinute()<10) Serial.print(F("0")); Serial.print(rtcGetMinute(),DEC); Serial.print(F(":"));
  if(rtcGetSecond()<10) Serial.print(F("0")); Serial.print(rtcGetSecond(),DEC);
  Serial.println();

  Udp.flush(); //in case of extraneous(?) data
  //Udp.stop() was formerly here
  ntpGoing = 0;
} //end fn checkNTP

bool networkNTPOK(){
  //Serial.print(F("NTP is ")); Serial.print(millis()-ntpSyncLast < NTPOK_THRESHOLD?F("OK: "):F("stale: ")); Serial.print(millis()-ntpSyncLast); Serial.print(F("ms old (vs limit of ")); Serial.print(NTPOK_THRESHOLD); Serial.print(F("ms)")); Serial.println();
  return (millis()-ntpSyncLast < NTPOK_THRESHOLD);
}

unsigned long adminInputLast = 0; //for noticing when the admin page hasn't been interacted with in 2 minutes, so we can time it (and AP if applicable) out

void networkStartAdmin(){
  adminInputLast = millis();
  //TODO display should handle its own code for displaying a type of stuff
  if(WiFi.status()!=WL_CONNECTED){
    networkStartAP();
    //displayInt(7777); delay(2500); //TODO reimplement similar to versionRemain/date pagination
    Serial.println(F("Admin started at 7.7.7.7"));
  } else { //use existing wifi
    IPAddress theip = WiFi.localIP();
    // displayInt(theip[0]); delay(2500);
    // displayInt(theip[1]); delay(2500);
    // displayInt(theip[2]); delay(2500);
    // displayInt(theip[3]); delay(2500);
    Serial.print(F("Admin started at "));
    Serial.println(theip);
  }
  //displayClear();
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
    if(adminInputLast==0) { client.flush(); client.stop(); Serial.print(F("Got a client but ditched it because last admin input was over ")); Serial.print(ADMIN_TIMEOUT); Serial.println(F("ms ago.")); return; }
    else { Serial.print(F("Last admin input was ")); Serial.print(millis()-adminInputLast); Serial.print(F("ms ago which is under the limit of ")); Serial.print(ADMIN_TIMEOUT); Serial.println(F("ms.")); }
    
    adminInputLast = millis();
    
    String currentLine = ""; //we'll read the data from the client one line at a time
    int requestType = 0;
    bool newlineSeen = false;

    if(client.connected()){
      while(client.available()){ //if there's bytes to read from the client
        char c = client.read();
        Serial.write(c); //DEBUG
        
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
        client.print(F("<!DOCTYPE html><html><head><title>Clock Settings</title><style>body { background-color: #eee; color: #222; font-family: system-ui, -apple-system, sans-serif; font-size: 18px; line-height: 1.3em; margin: 1.5em; position: absolute; } a { color: #33a; } ul { padding-left: 9em; text-indent: -9em; list-style: none; } ul li { margin-bottom: 0.8em; } ul li * { text-indent: 0; padding: 0; } ul li label:first-child { display: inline-block; width: 8em; text-align: right; padding-right: 1em; font-weight: bold; } ul li.nolabel { margin-left: 9em; } input[type='text'],input[type='submit'],select { border: 1px solid #999; margin: 0.2em 0; padding: 0.1em 0.3em; font-size: 1em; font-family: system-ui, -apple-system, sans-serif; } @media only screen and (max-width: 550px) { ul { padding-left: 0; text-indent: 0; } ul li label:first-child { display: block; width: auto; text-align: left; padding: 0; } ul li.nolabel { margin-left: 0; }} .saving { color: #66d; } .ok { color: #3a3; } .error { color: #c53; } .explain { font-size: 0.85em; line-height: 1.3em; color: #666; } @media (prefers-color-scheme: dark) { body { background-color: #222; color: #ddd; } a { color: white; } #result { background-color: #373; color: white; } input[type='text'],select { background-color: #444; color: #ddd; } .explain { color: #999; } }</style><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'></head><body><h2 style='margin-top: 0;'>Clock Settings</h2><p id='loading'>Loading&hellip;</p><div id='content' style='display: none;'><ul>"));
        client.print(F("<li><label>Wi-Fi</label><form id='wform' style='display: inline;' onsubmit='save(this); return false;'><select id='wtype' onchange='wformchg()'><option value=''>None</option><option value='wpa'>WPA</option><option value='wep'>WEP</option></select><span id='wa'><br/><input type='text' id='wssid' name='wssid' placeholder='SSID (Network Name)' autocomplete='off' onchange='wformchg()' onkeyup='wformchg()' value='")); String wssid2 = wssid; wssid2.replace("'","&#39;"); client.print(wssid2); client.print(F("' /><br/><input type='text' id='wpass' name='wpass' placeholder='Password/Key' autocomplete='off' onchange='wformchg()' onkeyup='wformchg()' value='")); String wpass2 = wpass; wpass2.replace("'","&#39;"); client.print(wpass2); client.print(F("' /></span><span id='wb'><br/><label for='wki'>Key Index</label> <select id='wki' onchange='wformchg()'>")); for(char i=0; i<=4; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("' ")); client.print(wki==i?F("selected"):F("")); client.print(F(">")); if(i==0) client.print(F("Select")); else client.print(i,DEC); client.print(F("</option>")); } client.print(F("</select></span><br/><input id='wformsubmit' type='submit' value='Save' style='display: none;' /></form></li>"));
        client.print(F("<li><label>Last sync</label>As of page load time: [sync state]</li>"));
        client.print(F("<li><label>Sync frequency</label><select id='syncfreq' onchange='save(this)'><option value='min'>Every minute</option><option value='hr'>Every hour (at min :59)</option></select></li>"));
        client.print(F("<li><label>NTP packets</label><select id='ntpok' onchange='save(this)'><option value='y'>Yes (normal)</option><option value='n'>No (for dev/testing)</option></select></li>"));
        //client.print(F("<li><label>Brightness</label><select id='bright' onchange='save(this)'><option value='3'>High</option><option value='2'>Medium</option><option value='1'>Low</option></select></li>"));
        
        //Date
        //Time
        //Alarm time
        //Alarm on CHECK
        //Alarm skip CHECK
        //Day count enabled BITMASK
            // const unsigned int FN_TIMER = 1<<0; //1
            // const unsigned int FN_DAYCOUNT = 1<<1; //2
            // const unsigned int FN_SUN = 1<<2; //4
            // const unsigned int FN_WEATHER = 1<<3; //8
        //Day count down/up
        //Day count month
        //Day count date
        //Function preset ???????
        
        client.print(F("<li><label>Time format</label><select id='b16' onchange='save(this)'>")); for(char i=1; i<=2; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(16,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 1: client.print(F("12-hour")); break;
          case 2: client.print(F("24-hour")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>For time-of-day display only. Setting times is always done in 24-hour.</span></li>"));
          
        client.print(F("<li><label>Date format</label><select id='b17' onchange='save(this)'>")); for(char i=1; i<=5; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(17,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 1: client.print(F("month/date/weekday")); break;
          case 2: client.print(F("date/month/weekday")); break;
          case 3: client.print(F("month/date/year")); break;
          case 4: client.print(F("date/month/year")); break;
          case 5: client.print(F("year/month/date")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>The weekday is displayed as a number from 0 (Sunday) to 6 (Saturday). Four-tube clocks will display only the first two values in each of these options.</span></li>"));
          
        client.print(F("<li><label>Auto-date</label><select id='b18' onchange='save(this)'>")); for(char i=0; i<=3; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(18,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Never")); break;
          case 1: client.print(F("Date instead of seconds")); break;
          case 2: client.print(F("Full date at :30 seconds (instant)")); break;
          case 3: client.print(F("Full date at :30 seconds (scrolling)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
          
        client.print(F("<li><label>Leading zeros</label><select id='b19' onchange='save(this)'>")); for(char i=0; i<=1; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(19,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("No (1:23)")); break;
          case 1: client.print(F("Yes (01:23)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select></li>"));
        
        //Digit fade: number
          
        client.print(F("<li><label>Auto DST</label><a name='autodst' href='#'></a><select id='b22' onchange='save(this)'>")); for(char i=0; i<=6; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(22,false)==i) client.print(F(" selected")); client.print(F(">")); switch(i){
          case 0: client.print(F("Off")); break;
          case 1: client.print(F("Second Sunday in March to first Sunday in November (US/CA)")); break;
          case 2: client.print(F("Last Sunday in March to last Sunday in October (UK/EU)")); break;
          case 3: client.print(F("First Sunday in April to last Sunday in October (MX)")); break;
          case 4: client.print(F("Last Sunday in September to first Sunday in April (NZ)")); break;
          case 5: client.print(F("First Sunday in October to first Sunday in April (AU)")); break;
          case 6: client.print(F("Third Sunday in October to third Sunday in February (BZ)")); break;
          default: break; } client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>If you observe DST but your locale's rules are not represented here, leave this set to 0 and set the clock (and <a href='#utcoffset'>UTC offset</a>) manually.</span></li>"));

        client.print(F("<li><label>UTC offset</label><a name='utcoffset' href='#'></a><select id='b14' onchange='save(this)'>")); for(char i=52; i<=156; i++){ client.print(F("<option value='")); client.print(i,DEC); client.print(F("'")); if(readEEPROM(14,false)==i) client.print(F(" selected")); client.print(F(">"));
        char offseth = abs(i-100)/4;
        char offsetm = (abs(i-100)%4)*15;
        if(i<100) client.print(F("–")); else client.print(F("+"));
        client.print(offseth,DEC);
        client.print(F(":"));
        if(offsetm<10) client.print(F("0"));
        client.print(offsetm,DEC);
        client.print(F("</option>")); } client.print(F("</select><br/><span class='explain'>Your time zone's offset from UTC (non-DST). If you observe DST but set the clock manually rather than using the <a href='#autodst'>auto DST</a> feature, you must add an hour to the UTC offset during DST, or the sunrise/sunset times will be an hour early.</span></li>"));
          
        client.print(F("<li><label>Version</label>TBD</li>"));
        client.print(F("<li><label>Sync</label><a id='syncnow' value='' href='#' onclick='save(this)'>Sync now</a></li>"));
        
        //After replacing the below from formdev.php, replace " with \"
        client.print(F("</ul></div><script type='text/javascript'>function e(id){ return document.getElementById(id); } function save(ctrl){ if(ctrl.disabled) return; ctrl.disabled = true; let ind = ctrl.nextSibling; if(ind && ind.tagName==='SPAN') ind.parentNode.removeChild(ind); ind = document.createElement('span'); ind.innerHTML = '&nbsp;<span class=\"saving\">Saving&hellip;</span>'; ctrl.parentNode.insertBefore(ind,ctrl.nextSibling); let xhr = new XMLHttpRequest(); xhr.onreadystatechange = function(){ if(xhr.readyState==4){ ctrl.disabled = false; if(xhr.status==200 && !xhr.responseText){ if(ctrl.id=='wform'){ e('content').innerHTML = '<p class=\"ok\">Wi-Fi changes applied.</p><p>' + (e('wssid').value? 'Now attempting to connect to <strong>'+htmlEntities(e('wssid').value)+'</strong>.</p><p>If successful, the clock will display its IP address. To access this settings page again, connect to <strong>'+htmlEntities(e('wssid').value)+'</strong> and visit that IP address. (If you miss it, hold Select for 5 seconds to see it again.)</p><p>If not successful, the clock will display <strong>7777</strong>. ': '') + 'To access this settings page again, (re)connect to Wi-Fi network <strong>Clock</strong> and visit <a href=\"http://7.7.7.7\">7.7.7.7</a>.</p>'; clearTimeout(timer); } else { ind.innerHTML = '&nbsp;<span class=\"ok\">OK!</span>'; setTimeout(function(){ if(ind.parentNode) ind.parentNode.removeChild(ind); },1500); } } else ind.innerHTML = '&nbsp;<span class=\"error\">'+xhr.responseText+'</span>'; timer = setTimeout(timedOut, 120000); } }; clearTimeout(timer); xhr.open('POST', './', true); xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded'); if(ctrl.id=='wform'){ switch(e('wtype').value){ case '': e('wssid').value = ''; e('wpass').value = ''; case 'wpa': e('wki').value = '0'; case 'wep': default: break; } xhr.send('wssid='+e('wssid').value+'&wpass='+e('wpass').value+'&wki='+e('wki').value); } else { xhr.send(ctrl.id+'='+ctrl.value); } } function wformchg(initial){ if(initial) e('wtype').value = (e('wssid').value? (e('wki').value!=0? 'wep': 'wpa'): ''); e('wa').style.display = (e('wtype').value==''?'none':'inline'); e('wb').style.display = (e('wtype').value=='wep'?'inline':'none'); if(!initial) e('wformsubmit').style.display = 'inline'; } function timedOut(){ e('content').innerHTML = 'Clock settings page has timed out. Please hold Select for 5 seconds to reactivate it, then <a href=\"#\" onclick=\"location.reload();\">refresh</a>.'; } function htmlEntities(str){ return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/\"/g, '&quot;'); } wformchg(true); let timer = setTimeout(timedOut, 120000); document.getElementById('loading').remove(); document.getElementById('content').style.display = 'block';</script></body></html>"));
        //client.print(F(""));
      } //end get
      else { //requestType==2 - handle what was POSTed
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
          startNTP();
        } else {
          //poll for other special types
          //else eeprom stuff, key is loc - e.g. 16=0
          bool isInt = currentLine.startsWith(F('i')); //or b for byte
          int eqPos = currentLine.indexOf(F("="));
          int key = currentLine.substring(1,eqPos).toInt();
          int val = currentLine.substring(eqPos+1).toInt();
          writeEEPROM(key,val,isInt);
          switch(key){
            case 14: //utc offset
              //nothing
            case 22: //auto dst
              isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),rtcGetHour(),true); break;
          }
          updateDisplay();
        }
      } //end post
    } //end if requestType
    
    // switch(requestType){
    //   case 0: //display the full page
    //
    //     break;
    //   case 1: //sync frequency
    //     client.print(rtcChangeMinuteSync()? F("Now syncing every minute."): F("Now syncing every hour at minute 59."));
    //     break;
    //   case 2:
    //     client.print(networkToggleNTPTest()? F("Now preventing incoming NTP packets."): F("Now allowing incoming NTP packets."));
    //     break;
    //   case 3:
    //     client.print(F("Display brightness set to "));
    //     client.print(displayToggleBrightness());
    //     client.print(F("."));
    //     break;
    //   default:
    //     client.print(F("Error: unknown request.")); break;
    // }

    // The HTTP response ends with another blank line: TODO need this?
    client.println();
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
  checkNTP();
  checkClients();
  checkForWiFiStatusChange();
}
  
#else
//AVR - dummy functions
void initNetwork(){}
void cycleNetwork(){}
#endif

#endif
#endif