#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "firebase_client";

const char *FIREBASE_BASE_URL = "https://console.firebase.google.com/u/0/project/espbackendapp/database/espbackendapp-default-rtdb/data/~2F";

esp_err_t firebase_put(const char *project_db_url, const char *path, const char *json_payload)
{
    char url[256];
    snprintf(url, sizeof(url), "%s/%s.json", FIREBASE_BASE_URL, path); 
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "PUT status=%d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "PUT failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t firebase_get(const char *project_db_url, const char *path, char *out_buf, size_t out_len)
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