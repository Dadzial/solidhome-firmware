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
#define MQTT_TOPIC_AIR 		   "solidhome/api/air"
#define MQTT_TOPIC_HEARTBEAT   "solidhome/api/board/heartbeat"
#define HEARTBEAT_INTERVAL_MS  5000U
#define AIR_READ_INTERVAL_MS   3000U

static mqtt_client_t g_mqtt_client;
static uint32_t s_last_heartbeat_ms = 0;
static uint32_t s_last_air_read_ms = 0;

/** Sprzętowy licznik milisekund SysTick */
static volatile uint32_t g_systick_ms = 0;

void SysTick_Handler(void) {
	g_systick_ms++;
}

int main(void) {
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	BOARD_InitDebugConsole();
#endif

	/* konfiguracja sprzętowego zegara SysTick na przerwanie co 1 ms (1000 Hz) */
	SysTick_Config(SystemCoreClock / 1000U);

	/** polaczenie z siecia */
	wlan_init(LP_FLEXCOMM5_PERIPHERAL, AP_SSID, AP_PASSPHRASE, AP_SECURITY_MODE);
	while (wlan_get_state() != MWM_CONNECTED);
	wlan_state();

	lights_init();
	air_init();

	/* inicializacja i polaczenie z brokerem */
	mqtt_client_init(&g_mqtt_client, MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_CLIENT_ID);
	mqtt_client_subscribe(&g_mqtt_client, MQTT_TOPIC_LIGHTS_SET, lights_mqtt_handler);

	if (mqtt_client_connect(&g_mqtt_client) == 0) {
		mqtt_client_publish(&g_mqtt_client, MQTT_TOPIC_HEARTBEAT, "{\"status\":\"online\"}");
		s_last_heartbeat_ms = g_systick_ms;
	}

	PRINTF("Starting event loop...\r\n");

	while (1) {
		uint32_t now = g_systick_ms;

		mqtt_client_poll(&g_mqtt_client, now);

		 if (now - s_last_air_read_ms >= AIR_READ_INTERVAL_MS) {
			 if (air_read() && g_mqtt_client.connected) {
				 char air_payload[128];
				 snprintf(air_payload, sizeof(air_payload),
					"{\"temperature\":%.2f,\"pressure\":%.2f,\"humidity\":%.2f}",
					airData.temperature, airData.pressure, airData.humidity);
				 mqtt_client_publish(&g_mqtt_client, MQTT_TOPIC_AIR, air_payload);
			 }
			 s_last_air_read_ms = now;
		 }

		 if (g_mqtt_client.connected && (now - s_last_heartbeat_ms >= HEARTBEAT_INTERVAL_MS)) {
		    mqtt_client_publish(&g_mqtt_client, MQTT_TOPIC_HEARTBEAT, "{\"status\":\"online\"}");
		    s_last_heartbeat_ms = now;
		 }


	}

	return 0;
}
