#include "time_sync.h"
#include "uart_connection.h"

static const char* TAG = "time";

void time_init_sync(void)
{
    ESP_LOGI(TAG, "Inicjalizacja SNTP (Time Sync)");
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");

    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED); 
    
    esp_sntp_init();

    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
}

void time_getTime()
{
    time_t now;
    struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year < (2016 - 1900)) {
            ESP_LOGI(TAG, "Czekam na czas z Internetu...");
        }
        else 
        {
            uart_sendTime(&timeinfo);
        }

}