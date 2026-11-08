#include "lights.h"
#include <string.h>
#include "board.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include "wlan_mwm.h"
#include "cJSON.h"

LightStruct lightsOutputs[LIGHTS_COUNT] = {
	{ "living_room" , BOARD_INITLIGHTSPINS_OUT1_GPIO, BOARD_INITLIGHTSPINS_OUT1_GPIO_PIN, 0 },
	{ "kitchen"     , BOARD_INITLIGHTSPINS_OUT2_GPIO, BOARD_INITLIGHTSPINS_OUT2_GPIO_PIN, 0 },
	{ "boiler_room" , BOARD_INITLIGHTSPINS_OUT3_GPIO, BOARD_INITLIGHTSPINS_OUT3_GPIO_PIN, 0 },
	{ "bathroom"    , BOARD_INITLIGHTSPINS_OUT4_GPIO, BOARD_INITLIGHTSPINS_OUT4_GPIO_PIN, 0 },
	{ "hallway"     , BOARD_INITLIGHTSPINS_OUT5_GPIO, BOARD_INITLIGHTSPINS_OUT5_GPIO_PIN, 0 },
	{ "garage"      , BOARD_INITLIGHTSPINS_OUT6_GPIO, BOARD_INITLIGHTSPINS_OUT6_GPIO_PIN, 0 }
};

static char lights_bufferRX[RXD_BUFFER_LEN] = { 0 };

void lights_init(void) {
	BOARD_InitLIGHTSPins();
	for (int i = 0; i < LIGHTS_COUNT; i++) {
		GPIO_PinWrite(lightsOutputs[i].gpio, lightsOutputs[i].pin, lightsOutputs[i].state);
	}
}

void lights_sync_status(void) {
	http_GET("192.168.2.48/api/lights/status/hardware", lights_bufferRX, 3100);

	char *jsonBody = strstr(lights_bufferRX, "\r\n\r\n");
	if (jsonBody == NULL) {
		return;
	}
	jsonBody += 4;

	cJSON *root = cJSON_Parse(jsonBody);
	if (root == NULL) {
		return;
	}

	for (int i = 0; i < LIGHTS_COUNT; i++) {
		cJSON *item = cJSON_GetObjectItem(root, lightsOutputs[i].name);

		if (item != NULL) {
			int nodeState = item->valueint;

			if (nodeState != lightsOutputs[i].state) {
				lightsOutputs[i].state = nodeState;
				GPIO_PinWrite(lightsOutputs[i].gpio, lightsOutputs[i].pin, nodeState);
				PRINTF("Sync: %s set to %d\r\n", lightsOutputs[i].name, nodeState);
			}
		}
	}
	cJSON_Delete(root);
}
