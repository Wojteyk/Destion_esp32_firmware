// Simple DNS server to redirect all queries to the AP IP for captive portal
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start the DNS server task. It will respond to any DNS query with the
 * configured captive portal IP (192.168.4.1 by default).
 */
void dns_server_start(void);

/**
 * Stop the DNS server task.
 */
void dns_server_stop(void);

#ifdef __cplusplus
}
#endif
