#ifndef _NETIF_WASM_ADD_H_
#define _NETIF_WASM_ADD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int netif_wasm_add(const char* ipaddr, const char* netmask);

/* Status values returned by the nonblocking current-IP coordinator. */
#define NETIF_WASM_REFRESH_FAILED (-1)
#define NETIF_WASM_REFRESH_UNCHANGED 0
#define NETIF_WASM_REFRESH_CHANGED 1
#define NETIF_WASM_REFRESH_BUSY 2

/* Try the shared core guard before probing; BUSY never probes. */
extern int netif_wasm_refresh(void);

/* Snapshot values are in network byte order. The ordinary form briefly
 * acquires the lwIP core mutex; the _core_locked form requires that mutex to
 * already be held by the caller. `valid` is false until netif initialization. */
typedef struct netif_wasm_ip_snapshot {
  uint32_t ip_addr;
  uint32_t netmask;
  int valid;
} netif_wasm_ip_snapshot_t;

extern int netif_wasm_get_ip_snapshot(netif_wasm_ip_snapshot_t *snapshot);
extern void netif_wasm_get_ip_snapshot_core_locked(
    netif_wasm_ip_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* _NETIF_WASM_ADD_H_ */
