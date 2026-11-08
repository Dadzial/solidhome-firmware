#ifndef LIGHTS_H_
#define LIGHTS_H_

#include <stdint.h>
#include "fsl_gpio.h"

#define LIGHTS_COUNT 6

typedef struct {
	const char *name;
	GPIO_Type *gpio;
	uint32_t pin;
	int state;
} LightStruct;


void lights_init(void);


void lights_sync_status(void);

#endif /* LIGHTS_H_ */
