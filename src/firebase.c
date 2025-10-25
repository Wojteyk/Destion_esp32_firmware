#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include "esp_crt_bundle.h"

#include "firebase.h"

#define MAX_RETRY_NUM 5
#define RETRY_DELAY_MS 500
static const char *TAG = "firebase_client";

const char *FIREBASE_BASE_URL = "https://espbackendapp-default-rtdb.europe-west1.firebasedatabase.app/";

static esp_err_t _firebase_put_htpp(const char *path, const char *json_payload)
{
    int retry_cnt =0;
    char url[256];
     esp_err_t err = ESP_FAIL;
    snprintf(url, sizeof(url), "%s/%s.json", FIREBASE_BASE_URL, path); 

    do{

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,     
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
   err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);

        if(status_code >=200 && status_code < 300){
            ESP_LOGI(TAG, "PUT Success status=%d", status_code);
            err = ESP_OK;
        } else {
            ESP_LOGW(TAG, "PUT failed (HTPP status %d)", status_code);
            err = ESP_FAIL;
        }        
    } else {
        ESP_LOGE(TAG, "PUT failed (transport): %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    if(err != ESP_OK && retry_cnt < MAX_RETRY_NUM){
        vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
    }

    retry_cnt++;
    }   while (err != ESP_OK && retry_cnt < MAX_RETRY_NUM);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PUT FAILED after %d attempts.", MAX_RETRY_NUM);
    }
    
    return err;
}

esp_err_t firebase_put_float_impl(const char *path, float value){
    char payload_str[32];
    snprintf(payload_str, sizeof(payload_str), "%.2f", value);

    return _firebase_put_htpp(path, payload_str);
}

esp_err_t firebase_put_int_impl(const char *path, int value){
    char payload_str[32];
    snprintf(payload_str, sizeof(payload_str), "%d", value);

    return _firebase_put_htpp(path, payload_str);
}

esp_err_t firebase_put_bool_impl(const char *path, bool value){
    char payload_str[8];

    const char *bool_string = value ? "true" : "false";
    snprintf(payload_str, sizeof(payload_str), "%s", bool_string);

    return _firebase_put_htpp(path, payload_str);
}

esp_err_t firebase_put_string_impl(const char *path, const char* value){
    return _firebase_put_htpp(path, value);
}

esp_err_t firebase_get( const char *path, char *out_buf, size_t out_len)
{
    char url[256];
    snprintf(url, sizeof(url), "%s/%s.json", FIREBASE_BASE_URL, path);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int len = esp_http_client_read_response(client, out_buf, out_len - 1);
        if (len > 0) out_buf[len] = '\0';
        ESP_LOGI(TAG, "GET status=%d len=%d", status, len);
    } else {
        ESP_LOGE(TAG, "GET failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}