#ifndef _NETIF_WASM_ADD_H_
#define _NETIF_WASM_ADD_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int netif_wasm_add(const char* ipaddr, const char* netmask);

/* Re-probe the host-side local IPv4 address and update netif_default.
 * Returns 1 when the address changed, 0 when it stayed the same, and -1 on
 * failure. */
extern int netif_wasm_refresh(void);
extern int netif_wasm_take_ip_changed(void);

#ifdef __cplusplus
}
#endif

#endif /* _NETIF_WASM_ADD_H_ */
