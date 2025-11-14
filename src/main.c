#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"

#include "dht11.h"
#include "hardware.h"
#include "wifi_provisioning.h"
#include "uart_connection.h"


static const char* TAG = "main";

void
app_main(void)
{
    ESP_LOGW(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    pc_switch_init();
    relay_init();
    dht11_init();
    uart_init();
    wifi_provisioning_start();

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM)); // making it more energy efficient

    xTaskCreate(dht_uart_task, "dht_uart_task", 4096, NULL, 5, NULL);
     xTaskCreate(uart_pc_receive_task, "uart_pc_receive_task", 4096, NULL, 5, NULL);
}