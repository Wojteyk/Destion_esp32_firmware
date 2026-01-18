#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "uart_connection.h"
#include "firebase.h"
#include "hardware.h"
#include "time_sync.h"

#define UART_PORT UART_NUM_1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD 115200
#define BUFF_SIZE 256

typedef enum {
    PC_CMD_OFF = 0,
    PC_CMD_ON  = 1
} pc_command_t;

static const char* TAG = "uart";
static QueueHandle_t uart_evt_queue = NULL;

void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, BUFF_SIZE * 2, 0, 20, &uart_evt_queue, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART initialized on TX=%d RX=%d", UART_TX_PIN, UART_RX_PIN);
}

void uart_event_task(void *pvParameters)
{
    uint8_t buf[BUFF_SIZE];
     uart_event_t event;

    while (1)
    {
        if (xQueueReceive(uart_evt_queue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                {
                    int len = uart_read_bytes(UART_PORT, buf, event.size, portMAX_DELAY);
                    if (len > 0) {
                        buf[len] = '\0';

                        if (strstr((char*)buf, "PC:On")) {
                            firebase_put("CONTROLS/pc_switch", (bool)true);
                        } 
                        else if (strstr((char*)buf, "PC:Off")) {
                            firebase_put("CONTROLS/pc_switch", (bool)false);
                        }
                        else if (strstr((char*)buf, "PC:?")) {
                            uart_pc_callback(relay_state);
                        }
                        else if (strstr((char*)buf, "TIME:?")) {
                            time_getTime();
                        }
                        if (strstr((char*)buf, "L:On")) {
                            firebase_put("CONTROLS/light", (bool)true);
                        }
                        if (strstr((char*)buf, "L:Off")) {
                            firebase_put("CONTROLS/light", (bool)false);
                        }  
                        else
                        {
                            ESP_LOGW(TAG, "Nieznana komenda: '%s'", buf);
                        }
                    }
                }
                break;

                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_evt_queue);
                break;

                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART RX buffer full");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_evt_queue);
                break;
                
                case UART_BREAK:
                    ESP_LOGW(TAG, "UART break detected (device disconnected?)");
                break;

                default:
                    ESP_LOGW(TAG, "Unhandled UART event: %d", event.type);
                break;
            }
        }
    }
}

void uart_sendSensorsData(float temperature, float humidity)
{
    char buffer[BUFF_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "T:%.2f;H:%.2f\n", temperature, humidity);

    if(len > 0)
    {
        uart_write_bytes(UART_PORT, buffer, len);
        ESP_LOGI(TAG, "Sent: %s", buffer);
    }
}

void uart_pc_callback(bool state){
    char buffer[BUFF_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "PC:%d\n", state);
    if(len > 0){
        uart_write_bytes(UART_PORT, buffer, len);
        ESP_LOGI(TAG, "Sent: %s", buffer);
    }
}

void uart_sendTime(struct tm *timeinfo)
{
    char buffer[BUFF_SIZE];
    int len = strftime(buffer, sizeof(buffer), "H%H:M%M:S%S\n", timeinfo);
    if(len >  0)
    {
        uart_write_bytes(UART_PORT, buffer, len);
        ESP_LOGI(TAG, "Sent: %s", buffer);
    }
}

void uart_sendLightState(bool state){
    char buffer[BUFF_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "L:%d\n", state);
    if(len > 0){
        uart_write_bytes(UART_PORT, buffer, len);
        ESP_LOGI(TAG, "Sent: %s", buffer);
    }
}