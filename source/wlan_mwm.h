/*
 * wlan_mwm.h
 *
 *  Created on: 19 lut 2022
 *      Author: daniel
 */

#ifndef WLAN_MWM_H_
#define WLAN_MWM_H_

#include <stdio.h>
#include "fsl_debug_console.h"
#include "serial_mwm.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Task priorities. */
#define MAIN_TASK_PRIORITY (configMAX_PRIORITIES - 1)

#define MAX_WLAN_SCAN		5
#define RXD_BUFFER_LEN		1400
#define TXD_BUFFER_LEN		1024
#define HOSTNAME_LEN		128
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

int wlan_get_state();
void wlan_state();
void wlan_scan();
void wlan_connect();
void wlan_disconnect();
void wlan_reboot();
void wlan_config(char *ap_ssid, char *ap_passphrase , char *ap_security_mode);
void wlan_init(LPUART_Type *base, char *ap_ssid, char *ap_passphrase , char *ap_security_mode);
void http_POST(const char *reqdata, char *respdata, char *contdata,int port) ;
void http_GET(const char *reqdata, char *respdata,int port) ;
void http_head_parser(char *headData, char *parseVal, char* keyVal) ;

#endif /* WLAN_MWM_H_ */
