#include "esp_err.h"

#pragma once

/**
 * Write JSON payload to Realtime Database at given path.
 * Example path: "devices/device1/state"
 *
 * Returns ESP_OK on success.
 */
esp_err_t firebase_put(const char *path, const char *json_payload);

/**
 * Read JSON from Realtime Database at given path into out_buf.
 * out_len is the size of out_buf; result is null-terminated on success.
 *
 * Returns ESP_OK on success.
 */
esp_err_t firebase_get( const char *path, char *out_buf, size_t out_len);
