#include "device.h"

#include <stdlib.h>

// RTOS items
static TimerHandle_t sensor_timer;
extern QueueHandle_t event_queue;

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;

static const char *TAG = "DEVICE";

void sensor_set_led(int sensor_reading);

// Function to handle incoming information from RainMaker App/Cloud
void hardware_update(event_packet_t event)
{
    switch (event.device) {
        case DEVICE_ONBOARD_LED:
            onboard_led_set(event.data_onboard_led);
            break;
        case DEVICE_EXT_LED:
            ext_led_set(event.data_ext_led);
            break;
        case DEVICE_SENSOR:
            // Do nothing
            break;
        default:
            ESP_LOGE(TAG, "Unknown device type");
            break;
    }
    return;
}


/******************************************************
 * On Board LED functions
******************************************************/
static bool current_led_state;

/** 
 * @brief Callback for boot button press
 * 
*/
static void push_btn_callback(void *arg)
{
    // Note that the global variable current_led_state is modified throught the function
    onboard_led_set(!current_led_state);

    event_packet_t led_data_to_app = {
        .direction = ESP_TO_APP,
        .device = DEVICE_ONBOARD_LED,
        .data_onboard_led = current_led_state,
    };

    xQueueSend(event_queue, &led_data_to_app, 0);
}

esp_err_t onboard_led_init(void)
{
    // Configure boot button
    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);

    if (btn_handle) {
        /* Register a callback for a button tap (short press) event */
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_callback, NULL);
        /* Register Wi-Fi reset and factory reset functionality on same button */
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }

    // Configure on board led
    ws2812_led_init();
    onboard_led_set(INIT_ONBOARD_LED_STATE);
    current_led_state = INIT_ONBOARD_LED_STATE;

    return ESP_OK;
}

void onboard_led_set(bool isLedOn)
{   
    current_led_state = isLedOn;

    if (isLedOn) {
        ws2812_led_set_rgb(DEFAULT_RED, DEFAULT_GREEN, DEFAULT_BLUE);
    } else {
        ws2812_led_clear();
    }
}

/******************************************************
 * LED functions
******************************************************/

esp_err_t ext_led_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = ((uint64_t)1 << EXT_LED_PIN),
    };
    return gpio_config(&io_conf);
}

esp_err_t ext_led_set(bool isLedOn)
{
    event_packet_t ext_led_data_to_app = {
        .direction = ESP_TO_APP,
        .device = DEVICE_EXT_LED,
        .data_ext_led = isLedOn,
    };

    xQueueSend(event_queue, &ext_led_data_to_app, 0);
    return gpio_set_level(EXT_LED_PIN, isLedOn);
}

/******************************************************
 * Sensor functions
******************************************************/

float map_range(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float get_sensor_reading(void)
{
    // Hint: Something feels off here hmmmmm
    int rand_num = rand() % 20;
    return (float)(rand_num + 60);

    // Read values
    int raw_value = -1;
    adc_oneshot_read(adc_handle, ADC_PIN, &raw_value);
    float sensor_val = map_range((float)raw_value, 0, ADC_RAW_MAX, 0, SENSOR_RANGE);
    ESP_LOGI(TAG, "Raw ADC value: %d, percentage value: %f", raw_value, sensor_val);
    // Turn off power to sensor
    return sensor_val;
}

static void sensor_update(TimerHandle_t handle)
{
    float reading = get_sensor_reading();

    event_packet_t sensor_data_to_app = {
        .direction = ESP_TO_APP,
        .device = DEVICE_SENSOR,
        .data_sensor = reading,
    };

    xQueueSend(event_queue, &sensor_data_to_app, 0);
}

void sensor_set_led(int sensor_reading)
{
    return;
    // TODO: insert how to control LED based on sensor reading
    // Hint: can use ENV_BRIGHT and ENV_DARK from device.h
}

esp_err_t sensor_init(void)
{
    // ADC for analog read
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT,
    };

    adc_oneshot_new_unit(&adc_config, &adc_handle);

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };

    adc_oneshot_config_channel(adc_handle, ADC_PIN, &channel_config);

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };

    adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);

    // Start timer to trigger every reporting interval 
    sensor_timer = xTimerCreate("sensor_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, sensor_update);
    if (sensor_timer) {
        xTimerStart(sensor_timer, 0);
        return ESP_OK;
    }
    return ESP_FAIL;
}

