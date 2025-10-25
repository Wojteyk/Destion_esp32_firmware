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
#define RELAY_GPIO_PIN 19 
#define RELAY_ON       1 
#define RELAY_OFF      0

extern void set_relay_state(const char *json_payload);

void set_relay_state(const char *json_payload) {
    if (json_payload == NULL) return;

    if (strstr(json_payload, "true") != NULL) {
        gpio_set_level(RELAY_GPIO_PIN, RELAY_ON);
        ESP_LOGI("RELAY", "RELAY SET HIGH.");
    } 
    else if (strstr(json_payload, "false") != NULL ) {
        gpio_set_level(RELAY_GPIO_PIN, RELAY_OFF);
        ESP_LOGI("RELAY", "RELAY SET LOW.");
    } else {
        ESP_LOGE("RELAY", "Received unrecognised payload: %s", json_payload);
    }
}

void relay_init(void) {
    gpio_reset_pin(RELAY_GPIO_PIN);
    gpio_set_direction(RELAY_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO_PIN, RELAY_OFF);
}

void fireabse_dht11_task(void *pvParameters){

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
    relay_init();
    dht11.dht11_pin = 5;

   
    wifi_provisioning_start();

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM)); // making it more energy efficient

    xTaskCreate(
        fireabse_dht11_task,
        "DHT11_Firebase",
        4096,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        firebase_switch_stream_task, 
        "FirebaseStream", 
        8192, 
        NULL, 
        7,     
        NULL
    );

}