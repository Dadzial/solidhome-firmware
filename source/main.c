/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    main.c
 * @brief   Application entry point for SolidHome.
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
#include "mqtt_client.h"

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
#define AP_SSID          ""
#define AP_PASSPHRASE    ""
#define AP_SECURITY_MODE "4"

#define MQTT_BROKER_HOST       "broker.emqx.io"
#define MQTT_BROKER_PORT       1883
#define MQTT_CLIENT_ID         "solidhome_nxp_board"
#define MQTT_TOPIC_LIGHTS_SET  "solidhome/api/lights/set"

static mqtt_client_t g_mqtt_client;

int main(void) {
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	BOARD_InitDebugConsole();
#endif

	/** Polaczenie z siecia */
	wlan_init(LP_FLEXCOMM5_PERIPHERAL, AP_SSID, AP_PASSPHRASE, AP_SECURITY_MODE);
	while (wlan_get_state() != MWM_CONNECTED);
	wlan_state();

	lights_init();

	/* Inicializacja i polaczenie z brokerem */
	mqtt_client_init(&g_mqtt_client, MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_CLIENT_ID);

	if (mqtt_client_connect(&g_mqtt_client) == 0) {
		mqtt_client_subscribe(&g_mqtt_client, MQTT_TOPIC_LIGHTS_SET, lights_mqtt_handler);
	}

	PRINTF("Starting event loop...\r\n");

	uint32_t simulated_tick_ms = 0;

	while (1) {
		mqtt_client_poll(&g_mqtt_client, simulated_tick_ms);

		/* 50ms tick */
		SDK_DelayAtLeastUs(50000U, SystemCoreClock);
		simulated_tick_ms += 50;
	}

	return 0;
}
