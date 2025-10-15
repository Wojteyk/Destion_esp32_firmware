
#pragma once

#include "esp_event.h"


#define WIFI_SUCCESS 1 << 0
#define WIFI_FAILURE 1 << 1

static const char *TAG  = "WIFI";

esp_err_t connect_wifi();


