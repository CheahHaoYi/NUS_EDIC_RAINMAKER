#pragma once

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define INIT_ONBOARD_LED_STATE false
#define INIT_SENSOR_READING 0.0
#define INIT_EXT_LED_STATE false

// Direction of packet flow
enum {
    ESP_TO_APP = 0,
    APP_TO_ESP = 1,
};

// Device target
enum {
    DEVICE_ONBOARD_LED = 0, // Switch device
    DEVICE_EXT_LED, // Light device
    DEVICE_SENSOR, // Sensor device
}; 

// Packet structure
typedef struct {
    uint8_t direction;
    uint8_t device;
    float data_sensor;
    bool data_ext_led;
    bool data_onboard_led;
} event_packet_t;