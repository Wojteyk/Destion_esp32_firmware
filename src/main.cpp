#include <iostream>
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "cstring"
#include "json.hpp"
#include "esp_https_server.h"

#define MAX_RETRY_NUM 5
static const char* TAG = "main";
// Prosta strona HTML z formularzem do wpisania danych Wi-Fi
const char* html_form = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Wi-Fi Setup</title></head>
<body>
  <h1>Skonfiguruj Wi-Fi</h1>
  <form action="/connect" method="get">
    SSID (Nazwa sieci):<br>
    <input type="text" name="ssid"><br>
    Haslo:<br>
    <input type="password" name="password"><br><br>
    <input type="submit" value="Polacz">
  </form>
</body>
</html>
)rawliteral";


// Funkcja, która zostanie wywołana, gdy ktoś wejdzie na główną stronę (adres /)
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_form, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Funkcja, która obsłuży dane wysłane z formularza (na adres /connect)
static esp_err_t connect_get_handler(httpd_req_t *req) {
    char ssid[32] = {0};
    char password[64] = {0};
    char buf[100];
    
    // Pobierz cały ciąg zapytania z URL (np. "ssid=MojaSiec&password=MojeHaslo")
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            // Wyszukaj wartości dla kluczy 'ssid' i 'password'
            if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) != ESP_OK ||
                httpd_query_key_value(buf, "password", password, sizeof(password)) != ESP_OK) {
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
    }

    ESP_LOGI(TAG, "Otrzymano SSID: %s, Haslo: %s", ssid, password);

    // Skonfiguruj i połącz z Wi-Fi
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Odpowiedz przeglądarce, że próbujemy się połączyć
    httpd_resp_send(req, "<h1>Probuje polaczyc...</h1><p>Restart urzadzenia po sukcesie.</p>", HTTPD_RESP_USE_STRLEN);

    // Tutaj można dodać logikę do zatrzymania serwera WWW po udanym połączeniu
    // W naszym przypadku event handler Wi-Fi może zrestartować urządzenie
    
    return ESP_OK;
}

// Funkcja uruchamiająca serwer WWW
static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard; // Umożliwia dopasowanie wzorców

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler
        };
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t connect_uri = {
            .uri       = "/connect",
            .method    = HTTP_GET,
            .handler   = connect_get_handler
        };
        httpd_register_uri_handler(server, &connect_uri);
    }
    return server;
}



static void wifi_prov_handler(void *user_data, wifi_prov_cb_event_t event, void *event_data)
{
    switch(event)
    {
        case WIFI_PROV_START :
        ESP_LOGI(TAG,"[WIFI_PROV_START]");
        break;
        case WIFI_PROV_CRED_RECV :
        ESP_LOGI(TAG ,"cred : ssid : %s pass : %s",((wifi_sta_config_t*)event_data)->ssid,((wifi_sta_config_t*)event_data)->password);
        break;
        case WIFI_PROV_CRED_SUCCESS :
        ESP_LOGI(TAG,"prov success");
        wifi_prov_mgr_stop_provisioning();
        break;
        case WIFI_PROV_CRED_FAIL :
        ESP_LOGE(TAG,"credentials worng");
        wifi_prov_mgr_reset_sm_state_on_failure();
        break;
        case WIFI_PROV_END: 
        ESP_LOGI(TAG,"prov ended");
        wifi_prov_mgr_deinit();
        break;
        default : break;
    }
}
static void wifi_event_handler(void* event_handler_arg,
                                        esp_event_base_t event_base,
                                        int32_t event_id,
                                        void* event_data)
{
    static int retry_cnt =0 ;
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START :
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED :
                ESP_LOGE(TAG,"ESP32 Disconnected, retrying");
                retry_cnt ++;
                if(retry_cnt < MAX_RETRY_NUM)
                {
                    esp_wifi_connect();
                }
                else ESP_LOGE(TAG,"Connection error");
                break;
            default : break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        if(event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG,"station ip :"IPSTR,IP2STR(&event->ip_info.ip));
        }
    }
}
void wifi_hw_init(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handler,NULL,&instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&wifi_event_handler,NULL,&instance_got_ip);
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
}

static void prov_start(void) {
    bool is_provisioned = false;
    
    // Sprawdź, czy dane są już w NVS
    wifi_config_t saved_config;
    if (esp_wifi_get_config(WIFI_IF_STA, &saved_config) == ESP_OK && strlen((const char*)saved_config.sta.ssid) > 0) {
        is_provisioned = true;
    }

    if (is_provisioned) {
        ESP_LOGI(TAG, "Urządzenie juz skonfigurowane. Laczenie z '%s'", saved_config.sta.ssid);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start(); // Rozpoczyna proces łączenia, który obsłuży event handler
    } else {
        ESP_LOGI(TAG, "Urządzenie nie skonfigurowane. Uruchamiam tryb AP i serwer WWW.");
        
        // Ustaw tryb AP (Access Point)
        esp_wifi_set_mode(WIFI_MODE_AP);
        
        // Skonfiguruj sieć AP
        wifi_config_t ap_config = {
            .ap = {
                .ssid = "ESP32_SETUP",
                .password = "1", // Bez hasła dla ułatwienia
                .ssid_len = strlen("ESP32_SETUP"),
                .authmode = WIFI_AUTH_OPEN,
                .max_connection = 1
            },
        };
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        
        // Uruchom Wi-Fi w trybie AP
        esp_wifi_start();
        
        // Uruchom serwer WWW
        start_webserver();
        ESP_LOGI(TAG, "Serwer WWW uruchomiony. Polacz sie z siecia 'ESP32_SETUP' i wejdz na 192.168.4.1");
    }
}



// static void prov_start(void)
// {
//     wifi_prov_mgr_config_t cfg = 
//     {
//         .scheme =wifi_prov_scheme_softap,
//         .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
//         .app_event_handler = wifi_prov_handler,
        
      
//     };
//     wifi_prov_mgr_init(cfg);
//     bool is_provisioned = 0;
//     wifi_prov_mgr_is_provisioned(&is_provisioned);
//     wifi_prov_mgr_disable_auto_stop(100);
//     if(is_provisioned)
//     {
//         ESP_LOGI(TAG,"Already provisioned");
//         esp_wifi_set_mode(WIFI_MODE_STA);
//         esp_wifi_start();
//     }
//     else
//     {
//         wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1,"abcd1234","PROV_ESP",NULL);
//         //print_qr();

//     }

// }
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
    // ESP_LOGW(TAG, "!!! FORCING NVS ERASE TO TEST PROVISIONING !!!");
    // ESP_ERROR_CHECK(nvs_flash_erase()); // Bezwarunkowe czyszczenie
    // ESP_ERROR_CHECK(nvs_flash_init()); 
    // ESP_ERROR_CHECK(ret);

    wifi_hw_init();
    prov_start();

     while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
    }
}