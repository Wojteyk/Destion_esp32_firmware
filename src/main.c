#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"

#include "sht40.h"
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
    sht40_init();
    uart_init();
    wifi_provisioning_start();

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM)); // making it more energy efficient

    xTaskCreate(sht40_uart_task, "sht40_uart_task", 4096, NULL, 7, NULL);
     xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 5, NULL);
}