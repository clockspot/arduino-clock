#ifndef NETWORK_H
#define NETWORK_H

bool networkSupported();
void checkForWiFiStatusChange();
void networkStartWiFi();
void networkStartAP();
void networkDisconnectWiFi();
unsigned long ntpSyncAgo();
void cueNTP();
int startNTP(bool synchronous);
bool checkNTP();
void clearNTPSyncLast();
void networkStartAdmin();
void networkStopAdmin();
void checkClients();
void initNetwork();
void cycleNetwork();

#endif //NETWORK_H