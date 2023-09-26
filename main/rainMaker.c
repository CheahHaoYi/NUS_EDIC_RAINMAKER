#include "rainMaker.h"

extern QueueHandle_t event_queue;

esp_rmaker_node_t *end_node; 
esp_rmaker_device_t *led_device; // On board LED
esp_rmaker_device_t *light_device; // External LED
esp_rmaker_device_t *sensor_device; // Sensor

static const char *TAG = "rainMaker";

/******************************************************
 * Rainmaker configuration functions
******************************************************/

void wifi_init()
{
    /* Initialize Non-Volatile Storage. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init() */
    app_wifi_init();
}

void wifi_start()
{
    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    esp_err_t err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}

esp_rmaker_node_t* rainMaker_init() 
{   
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_console_init();

    esp_rmaker_config_t rmaker_config = {
        .enable_time_sync = false,
    };

    end_node = esp_rmaker_node_init(&rmaker_config, 
        "NUS EDIC Workshop", "Multiple Devices");

    if (!end_node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
    ESP_LOGI(TAG, "RainMaker initialization complete, please add devices & services");
    return end_node;
}

void rainMaker_start()
{
    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();
    
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    return;
}

void rainMaker_update(event_packet_t event) 
{   
    switch (event.device) {
        case DEVICE_ONBOARD_LED:
            // Update the switch device (on-board LED)
            // TODO: figure out the parameters
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(led_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(event.data_onboard_led)
            );
            break;
        case DEVICE_EXT_LED:
            // Update the light device (external LED)
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(event.data_ext_led)
            );
            break;
        case DEVICE_SENSOR:
            // Update the sensor device
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(sensor_device, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(event.data_sensor)
            );
            break;
        default:
            break;
    }
    return;
}

/******************************************************
 * On Board LED functions
******************************************************/
esp_err_t rm_add_dummy()
{
    esp_rmaker_device_t* dummy = esp_rmaker_device_create("Dummy Device", NULL, NULL);
    
    esp_rmaker_device_add_cb(dummy, NULL, NULL);
    esp_rmaker_device_add_attribute(dummy, "Hello", "from ESP32!");
    esp_rmaker_node_add_device(end_node, dummy); 
    return ESP_OK;
}

/**
 * @brief Callback to handle commands received from Rainmaker cloud on the LED device.
*/
static esp_err_t led_callback(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param_label,
    const esp_rmaker_param_val_t value, void *private_data, esp_rmaker_write_ctx_t *context)
{
    if (context) {
        ESP_LOGI(TAG, "Received write request from %s", esp_rmaker_device_cb_src_to_str(context->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param_label);
    ESP_LOGI(TAG, "Device: %s, Param: %s", device_name, param_name);

    // If turning the light on/off
    if (strcmp(param_name, PARAM_NAME_ON_OFF) == 0) {
        ESP_LOGI(TAG, "Received on-off: %s", value.val.b ? "true" : "false");
        // (Send to event queue)
        event_packet_t led_on_off = {
            .direction =  APP_TO_ESP,
            .device = DEVICE_ONBOARD_LED,
            .data_onboard_led = value.val.b,
        };
        xQueueSend(event_queue, &led_on_off, 0);

    } else if (strcmp(param_name, PARAM_NAME_LED) == 0) {
        ESP_LOGI(TAG, "Received brightness: %d", value.val.i);
        // TODO: Send to event queue, hardware side to differentiate between on/off and speed
    } else {
        ESP_LOGE(TAG, "Unknown param received");
        return ESP_OK; //Silently ignore unknown param
    }

    esp_rmaker_param_update_and_report(param_label, value);
    return ESP_OK;
}

esp_err_t rm_add_onboard_led(void)
{
    // The on-off parameter is a boolean (true/false)
    led_device = esp_rmaker_switch_device_create("Onboard LED", NULL, INIT_ONBOARD_LED_STATE);

    esp_rmaker_device_add_cb(led_device, led_callback, NULL);

    char* note = "Can you change this attribute?";
    esp_rmaker_device_add_attribute(sensor_device, "Task", note);

    return esp_rmaker_node_add_device(end_node, led_device);
}

/******************************************************
 * External LED functions
******************************************************/

/**
 * @brief Callback to handle commands received from Rainmaker cloud on the LED device.
*/
static esp_err_t light_sw_callback(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param_label,
    const esp_rmaker_param_val_t value, void *private_data, esp_rmaker_write_ctx_t *context)
{
    if (context) {
        ESP_LOGI(TAG, "Received write request from %s", esp_rmaker_device_cb_src_to_str(context->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param_label);
    ESP_LOGI(TAG, "Device: %s, Param: %s", device_name, param_name);

    // If turning the light on/off
    if (strcmp(param_name, PARAM_NAME_ON_OFF) == 0) {
        ESP_LOGI(TAG, "Received on-off: %s", value.val.b ? "true" : "false");
        // (Send to event queue)
        event_packet_t led_on_off = {
            .direction =  APP_TO_ESP,
            .device = DEVICE_EXT_LED,
            .data_ext_led = value.val.b,
        };
        xQueueSend(event_queue, &led_on_off, 0);
    } else if (strcmp(param_name, PARAM_NAME_LED) == 0) {
        ESP_LOGI(TAG, "Received brightness: %d", value.val.i);
        // TODO: Send to event queue, hardware side to differentiate between on/off and speed
    } else {
        ESP_LOGE(TAG, "Unknown param received");
        return ESP_OK; //Silently ignore unknown param
    }
    esp_rmaker_param_update_and_report(param_label, value);
    return ESP_OK;
}

esp_err_t rm_add_ext_led(void)
{
    // The on-off parameter is a boolean (true/false)
    light_device = esp_rmaker_lightbulb_device_create("External LED", NULL, INIT_EXT_LED_STATE);

    esp_rmaker_device_add_cb(light_device, light_sw_callback, NULL);

    const esp_rmaker_param_t *param = esp_rmaker_brightness_param_create(PARAM_NAME_LED, DEFAULT_LIGHT_BRIGHTNESS);
    esp_rmaker_device_add_param(light_device, param);

    esp_rmaker_device_add_attribute(light_device, "Serial Number", "1234");
    // Hint: add attribute here

    return esp_rmaker_node_add_device(end_node, light_device);
}

/******************************************************
 * Sensor functions
******************************************************/

esp_err_t rm_add_sensor(void)
{
    sensor_device = esp_rmaker_temp_sensor_device_create("Sensor", NULL, INIT_SENSOR_READING);
    char* note = "The sensor is wrongly labeled as temperature for now";
    esp_rmaker_device_add_attribute(sensor_device, "Note", note);

    return esp_rmaker_node_add_device(end_node, sensor_device);
}

