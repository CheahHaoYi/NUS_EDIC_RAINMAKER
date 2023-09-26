#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sdkconfig.h>

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_intr_alloc.h"
#include "driver/gpio.h"

#include <iot_button.h>
#include <ws2812_led.h>
#include <app_reset.h>

#include "packet.h"

#define BUTTON_ACTIVE_LEVEL   0

/* Pin for on-board led */
#define BUTTON_GPIO          0
/* Pin for external led */
#define EXT_LED_PIN       1

// Pin for sensor (analog read)
// refer to documentation
// Pin 5 is ADC unit 1, channel 4
#define ADC_UNIT  ADC_UNIT_1
#define ADC_PIN ADC_CHANNEL_4
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_BITWIDTH ADC_BITWIDTH_DEFAULT
#define ADC_RAW_MAX (4095)
#define SENSOR_RANGE (100)

// period at which sensor reading is updated (in seconds)
#define REPORTING_PERIOD (10)

// Value boundaries for moisture sensor
// The higher, the drier
#define ENV_BRIGHT 80 
#define ENV_DARK 60

/* These values correspoind to H,S,V = 120,100,10 */
#define DEFAULT_RED     0
#define DEFAULT_GREEN   25
#define DEFAULT_BLUE    0

/* To reset & display QR code after reset*/
#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

void hardware_update(event_packet_t event);

esp_err_t onboard_led_init(void);

void onboard_led_set(bool isLedOn);

esp_err_t ext_led_init(void);

esp_err_t ext_led_set(bool isLedOn);

float get_sensor_reading(void);

esp_err_t sensor_init(void);