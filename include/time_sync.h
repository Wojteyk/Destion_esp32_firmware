#pragma once

#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"

void time_init_sync(void);

void time_getTime();
