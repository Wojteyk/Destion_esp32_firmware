#include <iostream>
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

static const char* TAG = "main";
// The captive portal webserver and HTML are implemented in the provisioning
// module (`src/wifi_provisiong.c`).

extern "C" void app_main(void)
{

    ESP_LOGW(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Standardowa obsługa błędów
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // // DODAJ TEN FRAGMENT, ABY ZAWSZE CZYŚCIĆ PAMIĘĆ NA POTRZEBY TESTÓW
    ESP_LOGW(TAG, "!!! FORCING NVS ERASE TO TEST PROVISIONING !!!");
    ESP_ERROR_CHECK(nvs_flash_erase()); // Bezwarunkowe czyszczenie
    ESP_ERROR_CHECK(nvs_flash_init()); 
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi stack and start provisioning/captive portal if needed
    wifi_provisioning_start();

     while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
    }
}