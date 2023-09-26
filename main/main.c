#include <esp_log.h>

#include "packet.h"
#include "device.h"
#include "rainMaker.h"

static const char *TAG = "MAIN";
#define INITIAL_POWER_STATE false
#define INITIAL_SENSOR_READING 20.0

QueueHandle_t event_queue = NULL;

#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS)

void flash_led() 
{
    while (true) {
        onboard_led_set(true);
        DELAY(1000);
        onboard_led_set(false);
        DELAY(1000);
    }
}

void queue_processing()
{
    event_packet_t event = {0};

    while (true) {
        DELAY(1000);

        if (xQueueReceive(event_queue, &event, 100) == pdTRUE) {
            ESP_LOGI(TAG, "Receive dir (%d), dev (%d)", event.direction, event.device);
            
            if (event.direction == ESP_TO_APP) {
                rainMaker_update(event);
            } else if (event.direction == APP_TO_ESP) {
                hardware_update(event);
            }
        }
    }
}

void app_main()
{
    ESP_LOGI(TAG, "Start of main function");

    // Workshop Part 1: Simple blink on-board LED 
    onboard_led_init();
    flash_led();
    // Task: can you make the LED blink faster? or change the LED color?
    

    // Rainmaker initialization sequence:
    /**
     * 1. Initialize Wi-Fi
     * 2. Initialize RainMaker node
     * 3. Add devices and services
     * 4. Start RainMaker
     * 5. Start Wi-Fi
    */

    // Workshop Part 2: Wi-Fi Provisioning
    // wifi_init();
    // rainMaker_init();
    // rm_add_dummy();
    // rainMaker_start();
    // wifi_start();
    
    // Workshop Part 3: RainMaker & Toggle LED from App
        // Task: Add a "school name" attribute to the device
    // wifi_init();
    // rainMaker_init();
    // onboard_led_init();
    // rm_add_onboard_led();
    // rainMaker_start();
    // wifi_start();

    // Workshop Part 4: Add external led & toggle both
    // wifi_init();
    // rainMaker_init();
    // onboard_led_init();
    // rm_add_onboard_led();
    // ext_led_init();
    // rm_add_ext_led();
    // rainMaker_start();
    // wifi_start();

    // Workshop Part 5: Add Sensor
        // Task: Fix the sensor reading (why is it inconsistent? Hint: device.c)
    // wifi_init();
    // rainMaker_init();
    // onboard_led_init();
    // rm_add_onboard_led();
    // ext_led_init();
    // rm_add_ext_led();
    // sensor_init();
    // rm_add_sensor();
    // rainMaker_start();
    // wifi_start();

    // to test sensor reading, comment everything above and just run this
    // sensor_init();

    // Workshop Part 6: Putting everything together
        // Task:
            // 1. Use the sensor reading to control the LED
            // 2. Make the led turn on when the sensor reading is low
            // 3. (Advanced) Change the color of the LED based on the sensor reading

    // Create input processing queue
    event_queue = xQueueCreate(50, sizeof(event_packet_t));
    queue_processing(); 
}
