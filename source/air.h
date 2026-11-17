#ifndef AIR_H_
#define AIR_H_

#include <stdint.h>
#include <stdbool.h>
#include "fsl_gpio.h"

typedef struct {
	float temperature;
	float pressure;
	float humidity;
	bool valid;
} AirStruct;

extern AirStruct airData;

/** inicjalizacja czujnika */
bool air_init(void);

/** odczyt danych */
bool air_read(void);


#endif /* AIR_H_ */
