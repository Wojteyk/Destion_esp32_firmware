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
#include "dns_server.h"

#define MAX_RETRY_NUM 5
static const char* TAG = "main";


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

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    httpd_resp_send(req, "<h1>Probuje polaczyc...</h1><p>Restart urzadzenia po sukcesie.</p>", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}


static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard; 

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



// static void wifi_prov_handler(void *user_data, wifi_prov_cb_event_t event, void *event_data)
// {
//     switch(event)
//     {
//         case WIFI_PROV_START :
//         ESP_LOGI(TAG,"[WIFI_PROV_START]");
//         break;
//         case WIFI_PROV_CRED_RECV :
//         ESP_LOGI(TAG ,"cred : ssid : %s pass : %s",((wifi_sta_config_t*)event_data)->ssid,((wifi_sta_config_t*)event_data)->password);
//         break;
//         case WIFI_PROV_CRED_SUCCESS :
//         ESP_LOGI(TAG,"prov success");
//         wifi_prov_mgr_stop_provisioning();
//         break;
//         case WIFI_PROV_CRED_FAIL :
//         ESP_LOGE(TAG,"credentials worng");
//         wifi_prov_mgr_reset_sm_state_on_failure();
//         break;
//         case WIFI_PROV_END: 
//         ESP_LOGI(TAG,"prov ended");
//         wifi_prov_mgr_deinit();
//         break;
//         default : break;
//     }
// }


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
    
    // Check if wifi config is already saved
    wifi_config_t saved_config;
    if (esp_wifi_get_config(WIFI_IF_STA, &saved_config) == ESP_OK && strlen((const char*)saved_config.sta.ssid) > 0) {
        is_provisioned = true;
    }

    if (is_provisioned) {
        ESP_LOGI(TAG, "Urządzenie juz skonfigurowane. Laczenie z '%s'", saved_config.sta.ssid);
        esp_wifi_set_mode(WIFI_MODE_STA);
        // Ensure DNS server is stopped when in STA mode
        dns_server_stop();
        esp_wifi_start(); // Rconnect to normal wifi
    } else {
        ESP_LOGI(TAG, "Urządzenie nie skonfigurowane. Uruchamiam tryb AP i serwer WWW.");
        
        esp_wifi_set_mode(WIFI_MODE_AP);
        
        wifi_config_t ap_config = {
            .ap = {
                .ssid = "ESP32_SETUP",
                .password = "", 
                .ssid_len = strlen("ESP32_SETUP"),
                .authmode = WIFI_AUTH_OPEN,
                .max_connection = 1
            },
        };
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        
        esp_wifi_start();

         //xTaskCreate(dns_task, "dns_task", 4096, NULL, 5, &dns_task_handle); //moze tu nie wiem bo nic nie dziala 
        // Start the captive-portal DNS responder so connected clients are
        // redirected to the softAP IP (192.168.4.1)
        dns_server_start();
        
        start_webserver();
        ESP_LOGI(TAG, "Serwer WWW uruchomiony. Polacz sie z siecia 'ESP32_SETUP' i wejdz na 192.168.4.1");
    }
}

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

    wifi_hw_init();
    prov_start();

     while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
    }
}