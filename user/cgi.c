/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include <ip_addr.h>
#include "espmissingincludes.h"

int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	int state;
  int postState;
  
  if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	
  if (connData->method == POST) {
    len=httpdFindArg(connData->postBuff, "state", buff, sizeof(buff));
    if (len != 0) {
      postState = atoi(buff);
      ioSetLed(postState);
    }
  }
  
  state = ioGetLed();
	len=os_sprintf(buff, "{\"state\": %d}", state);
	httpdSend(connData, buff, len);
	
	return HTTPD_CGI_DONE;
}
/*
{
  status : {
    mode: 0,
    version: "1.0.0",
    station: {
      ip: "192.168.0.2",
      gateway: "192.168.0.1",
      mask: "192.168.0.255",
      ssid: "redstar-mini",
      password: "hapsnorkel",
      mac_adderss: "3892897adf",
      auth_mode: 4,
      status: 4,
      channel: 7
    },
    access_point: {
      ip: "192.168.0.2",
      gateway: "192.168.0.1",
      mask: "192.168.0.255",
      ssid: "redstar-mini",
      password: "hapsnorkel",
      mac_adderss: "3892897adf",
      auth_mode: 4,
      status: 4.
      channel: 7
    }
  }
}
*/
int ICACHE_FLASH_ATTR cgiStatus(HttpdConnData *connData) {
  int len, mode, status;
	char buff[1024];
  static struct station_config stconf;
  static struct softap_config softapconf;
  struct ip_info ipconfig;
  uint8 hwaddr[6];
  
  if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
  
  if (connData->method != GET) {
    return HTTPD_CGI_NOTFOUND;
  }
    
  httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
  
  mode = wifi_get_opmode();  
  len=os_sprintf(buff, "{\"status\": { \"mode\": %d, \"version\": \"%s\" ", mode, "1.0.0");
	httpdSend(connData, buff, len);
  
  if (mode & (1<<(STATION_MODE-1))) {
    wifi_station_get_config(&stconf);
    status=wifi_station_get_connect_status();
    wifi_get_ip_info(STATION_IF, &ipconfig);
    wifi_get_macaddr(STATION_IF, hwaddr);
    
    len=os_sprintf(buff, ", \"station\": { \"ip\": \"" IPSTR "\", \"gw\": \"" IPSTR "\", \"mask\": \"" IPSTR "\", \"ssid\": \"%s\", \"password\": \"%s\", \"mac_address\": \"" MACSTR "\", \"auth_mode\": %d, \"status\": %d, \"channel\": %d }",
      IP2STR(&ipconfig.ip),
      IP2STR(&ipconfig.netmask),
      IP2STR(&ipconfig.gw),
      (char*)stconf.ssid,
      (char*)stconf.password,
      MAC2STR(hwaddr),
      -1,//authmode,
      status,
      -1);//channel);
    httpdSend(connData, buff, len);
  }
  
  if (mode & (1<<(SOFTAP_MODE-1))) {
    wifi_softap_get_config(&softapconf);
    wifi_get_ip_info(SOFTAP_IF, &ipconfig);
    wifi_get_macaddr(SOFTAP_IF, hwaddr);
    
    len=os_sprintf(buff, ", \"access_point\": { \"ip\": \"" IPSTR "\", \"gw\": \"" IPSTR "\", \"mask\": \"" IPSTR "\", \"ssid\": \"%s\", \"password\": \"%s\", \"mac_address\": \"" MACSTR "\", \"auth_mode\": %d, \"channel\": %d }",
      IP2STR(&ipconfig.ip),
      IP2STR(&ipconfig.netmask),
      IP2STR(&ipconfig.gw),
      (char*)softapconf.ssid,
      (char*)softapconf.password,
      MAC2STR(hwaddr),
      softapconf.authmode,
      softapconf.channel);
    httpdSend(connData, buff, len);      
  }
  
  os_strcpy(buff, "}}");
	httpdSend(connData, buff, -1);
    
  return HTTPD_CGI_DONE;
}

/*os_memset(softapconf.ssid, 0, sizeof(softapconf.ssid));
      os_memset(softapconf.password, 0, sizeof(softapconf.password));
      os_sprintf((char *)softapconf.ssid, "WETL_991AB1");
      softapconf.max_connection = 255; // 1?
      softapconf.ssid_hidden = 0;
      softapconf.ssid_len = 11;
      wifi_softap_set_config(&softapconf);*/

//Template code for the led page.
void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
  int state;
	if (token==NULL) return;    

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
    state=ioGetLed();
		if (state) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
	}
	httpdSend(connData, buff, -1);
}

static long hitCounter=0;

//Template code for the counter on the index page.
void ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}
	httpdSend(connData, buff, -1);
}


//Cgi that reads the SPI flash. Assumes 512KByte flash.
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData) {
	int *pos=(int *)&connData->cgiData;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (*pos==0) {
		os_printf("Start flash download.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/bin");
		httpdEndHeaders(connData);
		*pos=0x40200000;
		return HTTPD_CGI_MORE;
	}
	//Send 1K of flash per call. We will get called again if we haven't sent 512K yet.
	espconn_sent(connData->conn, (uint8 *)(*pos), 1024);
	*pos+=1024;
	if (*pos>=0x40200000+(512*1024)) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}

