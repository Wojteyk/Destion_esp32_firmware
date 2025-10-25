#include <stdio.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "esp_https_server.h"

#include "dns_server.h"
#include "wifi_provisioning.h"
#include "firebase.h"
#include "dht11.h"

static const char* TAG = "main";
dht11_t dht11;

void dht11_fireabse_task(void){

    while (true)
    {
        dht11_read(&dht11, 5);
        firebase_put("DHT11/temperature",dht11.temperature);
        firebase_put("DHT11/humidity",dht11.humidity);
        vTaskDelay(pdMS_TO_TICKS(1000 * 60 * 5)); //5 minutes
    }
}

void app_main(void)
{
    ESP_LOGW(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Standardowa obsługa błędów
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    dht11.dht11_pin = 5;

    // Initialize WiFi stack and start provisioning/captive portal if needed
    wifi_provisioning_start();

    xTaskCreate(
        dht11_fireabse_task,
        "DHT11_Firebase",
        4096,
        NULL,
        5,
        NULL
    );

}