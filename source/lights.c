#include "lights.h"
#include <string.h>
#include "board.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include "cJSON.h"

LightStruct lightsOutputs[LIGHTS_COUNT] = {
	{ "living_room" , BOARD_INITLIGHTSPINS_OUT1_GPIO, BOARD_INITLIGHTSPINS_OUT1_GPIO_PIN, 0 },
	{ "kitchen"     , BOARD_INITLIGHTSPINS_OUT2_GPIO, BOARD_INITLIGHTSPINS_OUT2_GPIO_PIN, 0 },
	{ "boiler_room" , BOARD_INITLIGHTSPINS_OUT3_GPIO, BOARD_INITLIGHTSPINS_OUT3_GPIO_PIN, 0 },
	{ "bathroom"    , BOARD_INITLIGHTSPINS_OUT4_GPIO, BOARD_INITLIGHTSPINS_OUT4_GPIO_PIN, 0 },
	{ "hallway"     , BOARD_INITLIGHTSPINS_OUT5_GPIO, BOARD_INITLIGHTSPINS_OUT5_GPIO_PIN, 0 },
	{ "garage"      , BOARD_INITLIGHTSPINS_OUT6_GPIO, BOARD_INITLIGHTSPINS_OUT6_GPIO_PIN, 0 }
};

void lights_init(void) {
	BOARD_InitLIGHTSPins();
	for (int i = 0; i < LIGHTS_COUNT; i++) {
		GPIO_PinWrite(lightsOutputs[i].gpio, lightsOutputs[i].pin, lightsOutputs[i].state);
	}
	PRINTF("[LIGHTS] Initialized GPIO pins for %d channels\r\n", LIGHTS_COUNT);
}

void lights_mqtt_handler(const char *topic, const char *payload) {
	if (!payload) return;

	cJSON *root = cJSON_Parse(payload);
	if (root == NULL) {
		PRINTF("[LIGHTS] Failed to parse JSON payload\r\n");
		return;
	}

	cJSON *target_name = cJSON_GetObjectItem(root, "name");
	cJSON *target_state = cJSON_GetObjectItem(root, "state");

	if (target_name && target_name->valuestring && target_state) {
		int newState = target_state->valueint;
		for (int i = 0; i < LIGHTS_COUNT; i++) {
			if (strcmp(lightsOutputs[i].name, target_name->valuestring) == 0) {
				lightsOutputs[i].state = newState;
				GPIO_PinWrite(lightsOutputs[i].gpio, lightsOutputs[i].pin, newState);
				PRINTF("[LIGHTS] Target match: %s -> %d\r\n", lightsOutputs[i].name, newState);
				cJSON_Delete(root);
				return;
			}
		}
	}

	for (int i = 0; i < LIGHTS_COUNT; i++) {
		cJSON *item = cJSON_GetObjectItem(root, lightsOutputs[i].name);
		if (item != NULL) {
			int newState = item->valueint;
			if (newState != lightsOutputs[i].state) {
				lightsOutputs[i].state = newState;
				GPIO_PinWrite(lightsOutputs[i].gpio, lightsOutputs[i].pin, newState);
				PRINTF("[LIGHTS] Sync match: %s -> %d\r\n", lightsOutputs[i].name, newState);
			}
		}
	}

	cJSON_Delete(root);
}
