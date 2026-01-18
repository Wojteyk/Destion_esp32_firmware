#include "hardware.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "firebase.h"
#include "uart_connection.h"

#define RELAY_GPIO_PIN 22
#define RELAY_ON 1
#define RELAY_OFF 0
#define RELAY_IMPULSE_TIME_MS 500

#define DEBOUNCE_TIME_MS 3000
#define BUTTON_GPIO_PIN 18

#define WINDOW_GPIO_PIN 2
#define WINDOW_OPEN 1
#define WINDOW_CLOSE 0

static QueueHandle_t window_evt_queue = NULL;
static QueueHandle_t gpio_evt_queue = NULL;
bool relay_state = RELAY_OFF;
bool window_state = WINDOW_CLOSE;

// Interrupt service routine for button press
static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void IRAM_ATTR window_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(window_evt_queue, &gpio_num, NULL);
}

void window_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE, 
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << WINDOW_GPIO_PIN),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    window_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0);
    gpio_isr_handler_add(WINDOW_GPIO_PIN, window_isr_handler, (void*)WINDOW_GPIO_PIN);
}

void window_task(void* pvParameters)
{
    uint32_t io_num;

    for(;;)
    {
        if(xQueueReceive(window_evt_queue, &io_num, portMAX_DELAY))
        {
            int state = gpio_get_level(WINDOW_GPIO_PIN);
            if(state == 1 && state != window_state)
            {   
                window_state = state;
                ESP_LOGI("WINDOW_TAKS", "Window closed");
                firebase_put("CONTROLS/window", (bool)!state);
            }
            else if(state == 0 && state != window_state)
            {
                window_state = state;
                ESP_LOGI("WINDOW_TAKS", "Window opend");
                firebase_put("CONTROLS/window", (bool)!state);
            }
        }
    }
}

void
relay_init(void)
{
    gpio_reset_pin(RELAY_GPIO_PIN);
    gpio_set_direction(RELAY_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO_PIN, RELAY_OFF);
}

void
set_relay_state(const char* json_payload)
{
    if (json_payload == NULL || strstr(json_payload, "null") != NULL)
        return;

    if (strstr(json_payload, "true") != NULL)
    {
        if (!relay_state)
        {
            gpio_set_level(RELAY_GPIO_PIN, RELAY_ON);
            vTaskDelay(pdMS_TO_TICKS(RELAY_IMPULSE_TIME_MS));
            gpio_set_level(RELAY_GPIO_PIN, RELAY_OFF);
        }
        relay_state = RELAY_ON;
        uart_pc_callback(true);
        ESP_LOGI("RELAY", "RELAY SET HIGH.");
    }
    else if (strstr(json_payload, "false") != NULL)
    {
        if (relay_state)
        {
            gpio_set_level(RELAY_GPIO_PIN, RELAY_ON);
            vTaskDelay(pdMS_TO_TICKS(RELAY_IMPULSE_TIME_MS));
            gpio_set_level(RELAY_GPIO_PIN, RELAY_OFF);
        }
        relay_state = RELAY_OFF;
        uart_pc_callback(false);
        ESP_LOGI("RELAY", "RELAY SET LOW.");
    }
    else
    {
        ESP_LOGE("RELAY", "Received unrecognized payload: %s", json_payload);
    }
}

void
set_light_state(const char* json_payload)
{
    if (json_payload == NULL || strstr(json_payload, "null") != NULL)
        return;

    if (strstr(json_payload, "true") != NULL)
    {
        uart_sendLightState(true);
        ESP_LOGI("Light", "light turn on");
    }
    else if (strstr(json_payload, "false") != NULL)
    {
        uart_sendLightState(false);
        ESP_LOGI("Light", "light turn off");
    }
    else
    {
        ESP_LOGE("RELAY", "Received unrecognized payload: %s", json_payload);
    }
}

void
pc_switch_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Trigger on falling edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable pull-up resistor
    };

    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0);

    gpio_isr_handler_add(BUTTON_GPIO_PIN, button_isr_handler, (void*)BUTTON_GPIO_PIN);
}

// Task to handle button presses with debounce
void
button_handler_task(void* pvParameters)
{
    uint32_t io_num;
    uint64_t last_interrupt_time = 0;

    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {

            uint64_t current_time = esp_timer_get_time() / 1000; // Time in ms
            if (current_time - last_interrupt_time < DEBOUNCE_TIME_MS)
            {
                continue; // Ignore presses within debounce period
            }

            ESP_LOGI("BUTTON_TASK", "Button on GPIO %d pressed! Time: %llums", io_num,
                     current_time);
            
            firebase_put("CONTROLS/pc_switch", (bool)!relay_state);

            last_interrupt_time = current_time;
        }
    }
}
