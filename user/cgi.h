#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int ICACHE_FLASH_ATTR cgiGetLed(HttpdConnData *connData);
int cgiLed(HttpdConnData *connData);
int ICACHE_FLASH_ATTR cgiStatus(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);
int cgiReadFlash(HttpdConnData *connData);
void tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif