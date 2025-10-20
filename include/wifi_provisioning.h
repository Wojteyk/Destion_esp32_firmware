
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize and start WiFi provisioning / captive portal.
 * This will start the softAP, DNS captive responder and HTTP server when the
 * device is not yet provisioned. If credentials are present it will start
 * WiFi in STA mode and attempt to connect.
 */
void wifi_provisioning_start(void);

#ifdef __cplusplus
}
#endif


