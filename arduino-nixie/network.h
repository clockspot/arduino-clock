#ifndef NETWORK_H
#define NETWORK_H

//#ifndef __AVR__ //TODO better sensor

void checkForWiFiStatusChange();
void networkStartWiFi();
void networkStartAP();
void networkDisconnectWiFi();
unsigned long ntpSyncAgo();
void cueNTP();
int startNTP(bool synchronous);
bool checkNTP();
void networkStartAdmin();
void networkStopAdmin();
void checkClients();
void initNetwork();
void cycleNetwork();

//#endif //ifndef __AVR__

#endif //NETWORK_H