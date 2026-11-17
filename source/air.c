#include "air.h"
#include "bmp280.h"
#include "peripherals.h"
#include "fsl_debug_console.h"
#include <stdio.h>

/** parametry konfiguracji */
static BMP280_HandleTypedef s_bmp280_dev;
static bmp280_params_t s_bmp280_params;

/** parametry startowe */
AirStruct airData = {
	.temperature = 0.0f,
	.pressure = 0.0f,
	.humidity = 0.0f,
	.valid = false
};

bool air_init(void) {
	bmp280_init_default_params(&s_bmp280_params);
	s_bmp280_dev.i2c = LP_FLEXCOMM2_PERIPHERAL;

	/** incializacja czujnika */
	s_bmp280_dev.addr = BMP280_I2C_ADDRESS_0;
	if (!bmp280_init(&s_bmp280_dev, &s_bmp280_params)) {
		s_bmp280_dev.addr = BMP280_I2C_ADDRESS_1;
		if (!bmp280_init(&s_bmp280_dev, &s_bmp280_params)) {
			PRINTF("[AIR] ERROR: BMP280/BME280 not found \r\n");
			airData.valid = false;
			return false;
		}
	}

	PRINTF("[AIR] Sensor initialized ID: 0x%02X, Addr: 0x%02X (%s)\r\n",
		s_bmp280_dev.id,
		s_bmp280_dev.addr,
		(s_bmp280_dev.id == BME280_CHIP_ID) ? "BME280 with humidity" : "BMP280");
	airData.valid = true;
	return true;
}

bool air_read(void) {
	float raw_temp = 0.0f;
	float raw_press = 0.0f;
	float raw_hum = 0.0f;

	if (!airData.valid) {
		if (!air_init()) {
			return false;
		}
	}

	if (bmp280_read_float(&s_bmp280_dev, &raw_temp, &raw_press, &raw_hum)) {
		airData.temperature = raw_temp;
		airData.pressure = raw_press / 100.0f;
		airData.humidity = raw_hum;
		airData.valid = true;

		PRINTF("[AIR] Temperature: %.2f C | Pressure: %.2f hPa | Humidity: %.2f %%\r\n",
			airData.temperature, airData.pressure, airData.humidity);
		return true;
	} else {
		PRINTF("[AIR] Error from I2C!\r\n");
		return false;
	}
}

