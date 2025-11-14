#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "uart_connection.h"
#include "firebase.h"

#define UART_PORT UART_NUM_1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD 115200
#define BUFF_SIZE 128

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

    uart_driver_install(UART_PORT, BUFF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_evt_queue = xQueueCreate(10, sizeof(pc_command_t));

    ESP_LOGI(TAG, "UART initialized on TX=%d RX=%d", UART_TX_PIN, UART_RX_PIN);
}

void uart_pc_receive_task(void *pvParameters)
{
    uint8_t buf[BUFF_SIZE];
    pc_command_t cmd;

    for(;;)
    {
        int len = uart_read_bytes(UART_PORT, buf, sizeof(buf) - 1, pdMS_TO_TICKS(100));
        if(len > 0)
        {
            buf[len] = '\0';

            if(strstr((char *)buf, "PC:On")) {
                cmd = PC_CMD_ON;
            } else if(strstr((char *)buf, "PC:Off")) {
                cmd = PC_CMD_OFF;
            } else {
                continue; 
            }

            firebase_put("CONTROLS/pc_switch", (bool)cmd);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void send_dht_data(float temperature, float humidity)
{
    char buffer[BUFF_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "T:%.2f;H:%.2f\n", temperature, humidity);

    if(len > 0)
    {
        uart_write_bytes(UART_PORT, buffer, len);
        ESP_LOGI(TAG, "Sent: %s", buffer);
    }
}

void dht_uart_task(void *pvParameters)
{
    while(1)
    {
        int status = dht11_read(&dht11, 5);
        if(status == 0){
            send_dht_data(dht11.temperature, dht11.humidity);
        }
        else
        {
            ESP_LOGW(TAG, "DHT11 read error status = %d", status);
        }
        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 minutes
    }
}
