// #include "wifi_provisioning.h"
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_wifi.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include <wifi_provisioning/manager.h> 
// #include "wifi_provisioning/scheme_softap.h"

// #define PROV_POP "12345678"

// // static EventGroupHandle_t wifi_event_group;
// // static int s_retry_num = 0;



// // static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// // {
// //     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
// //     {
// //         ESP_LOGI(TAG, "Connectig to ap...");
// //         esp_wifi_connect();
// //     }else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
// //     {
// //         if(s_retry_num < MAX_FAILURES){
// //             ESP_LOGI(TAG, "Connectig to ap...");
// //             esp_wifi_connect();
// //             s_retry_num++;
// //         } else {
// //             xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
// //         }
// //     }
// // }

// static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// {
//     if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
//     {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
//         wifi_prov_mgr_stop_provisioning();
//     }
// }

// // esp_err_t connect_wifi()
// // {

// //     ESP_ERROR_CHECK(esp_netif_init());
// //     ESP_ERROR_CHECK(esp_event_loop_create_default());

// //     esp_netif_create_default_wifi_sta();

// //     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
// //     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

// //     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

// //     bool provisioned = false;
// //     ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));



// //     if(!provisioned)
// //     {
// //         ESP_LOGW(TAG, "Device not provisioned. Starting provisoning...");

// //         wifi_prov_mgr_config_t prov_cfg = {
// //             .scheme = wifi_prov_scheme_softap,
// //             .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
// //         };
// //         ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_cfg));

// //         ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
// //         WIFI_PROV_SECURITY_0,   // Bezpieczeństwo: Najprostsza opcja (bez szyfrowania wiadomości protokołu)
// //         NULL,                   // Parametry bezpieczeństwa: NULL, ponieważ używamy SECURITY_0
// //         "ESP32_SETUP_AP",       // service_name: SSID Hotspota, widoczny dla użytkownika
// //         "12345678"              // service_key: Hasło Hotspota
// //     ));

// //         ESP_LOGI(TAG, "Provisioning started");

// //         wifi_prov_mgr_wait();

// //         wifi_prov_mgr_deinit();
// //         ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
// //         ESP_ERROR_CHECK(esp_wifi_start());

// //     } else {
// //         ESP_LOGI(TAG, "Device already provisioned. Trying to connect to saved network...");

// //         ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
// //         ESP_ERROR_CHECK(esp_wifi_start());
// //     }

// //     ESP_LOGI(TAG, "Provisioning process finished. Restarting in 5s.");
// //     vTaskDelay(pdMS_TO_TICKS(5000));
// //     esp_restart();

// //     return WIFI_FAILURE;

// // }

// // Gdzieś w kodzie musisz mieć definicję TAG, np.
// // static const char *TAG = "WIFI_CONNECT";

// // Ta funkcja powinna być wywołana tylko raz w app_main
// void wifi_init_stack(void)
// {
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
// }


// // Właściwa funkcja do obsługi połączenia i provisioningu
// void connect_or_provision(void)
// {
//     // Rejestracja handlerów zdarzeń (powinno się to zrobić przed startem Wi-Fi)
//     // UWAGA: Potrzebujesz zdefiniowanych funkcji `wifi_event_handler` i `ip_event_handler`
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

//     bool provisioned = false;
//     ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

//     if (!provisioned)
//     {
//         ESP_LOGI(TAG, "Device not provisioned. Starting provisioning via SoftAP...");

//         wifi_prov_mgr_config_t prov_cfg = {
//             .scheme = wifi_prov_scheme_softap,
//             .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
//         };
//         ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_cfg));

//         // Start provisioningu - ta funkcja uruchamia Wi-Fi w trybie AP lub BLE
//         ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
//             WIFI_PROV_SECURITY_1, // Zalecane użycie security 1 dla lepszego bezpieczeństwa
//             "proof_of_possession_string",  // Dowolny, ale znany aplikacji ciąg znaków jako PoP
//             "ESP32_SETUP_AP",
//             NULL // Hasło nie jest potrzebne dla SoftAP przy security 1
//         ));

//         ESP_LOGI(TAG, "Provisioning started. Waiting for connection from a phone app...");

//         // Czekamy na zakończenie procesu przez użytkownika
//         wifi_prov_mgr_wait();

//         // Po zakończeniu, sprzątamy i restartujemy
//         wifi_prov_mgr_deinit();
//         ESP_LOGI(TAG, "Provisioning finished. Restarting in 5 seconds.");
//         vTaskDelay(pdMS_TO_TICKS(5000));
//         esp_restart();
//     }
//     else
//     {
//         ESP_LOGI(TAG, "Device already provisioned. Starting Wi-Fi in STA mode...");
        
//         // Urządzenie ma dane, więc po prostu uruchamiamy Wi-Fi w trybie stacji
//         ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//         ESP_ERROR_CHECK(esp_wifi_start());
//         // Aplikacja będzie teraz normalnie działać. Brak restartu!
//     }
// }