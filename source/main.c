/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    main.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "wlan_mwm.h"
#include "lights.h"
#include "air.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*-----------------------------------------------------------------------------
 AP_SECURITY_MODE:
 0 - Open
 1 - WEP (Open mode)
 2 - WEP (Shared mode)
 3 - WPA-PSK
 4 - WPA2-PSK
 9 - WPA3-SAE
 ----------------------------------------------------------------------------*/
#define AP_SSID          "your-wifi-ssid"
#define AP_PASSPHRASE    "your-wifi-password"
#define AP_SECURITY_MODE "4"

int main(void) {
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	BOARD_InitDebugConsole();
#endif

	wlan_init(LP_FLEXCOMM5_PERIPHERAL, AP_SSID, AP_PASSPHRASE, AP_SECURITY_MODE);
	while (wlan_get_state() != MWM_CONNECTED);
	wlan_state();

	lights_init();

	PRINTF("SolidHome Autonomous Sync Started...\r\n");

	lights_sync_status(); // first sync

	while (1) {
		lights_sync_status();
		SDK_DelayAtLeastUs(500000U, SystemCoreClock);
	}

	return 0;
}
