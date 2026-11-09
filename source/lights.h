#ifndef LIGHTS_H_
#define LIGHTS_H_

#include <stdint.h>
#include "fsl_gpio.h"

/** licznik ilosci swiatel */
#define LIGHTS_COUNT 6

/** struktura swiatel */
typedef struct {
	const char *name;
	GPIO_Type *gpio;
	uint32_t pin;
	int state;
} LightStruct;

extern LightStruct lightsOutputs[LIGHTS_COUNT];


/** Inicjalizacja swiateł wedlug struktury */
void lights_init(void);

/** hander do manipulacji swiatel z mqtt posiada dwa ify gdy wlaczamy pojedynczo lub wszyskie na raz */
void lights_mqtt_handler(const char *topic, const char *payload);

#endif /* LIGHTS_H_ */
