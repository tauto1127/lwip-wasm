#include "netif_wasm_add.h"
#include "netif_wasm_get.h"
#include "lwipopts.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wasi_socket_ext.h>

#define DISCOVERY_MULTICAST_IP "239.255.0.1"
#define DISCOVERY_MULTICAST_PORT 7400
#ifndef LWIP_NETIF_SOCKET
#define LWIP_NETIF_SOCKET socket
#define LWIP_NETIF_CONNECT connect
#define LWIP_NETIF_GETSOCKNAME getsockname
#define LWIP_NETIF_CLOSE close
#endif

static struct netif netif_wasm;
struct netif* netif_default = NULL;

static int
probe_local_ip(in_addr_t *local_ip)
{
  int socket_fd = -1;
  struct sockaddr_in discovery = {0};
  struct sockaddr_in local = {0};
  socklen_t local_length = sizeof(local);
  int result = -1;

  socket_fd = LWIP_NETIF_SOCKET(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "netif_wasm: socket() failed: errno=%d\n", errno);
    return -1;
  }

  discovery.sin_family = AF_INET;
  discovery.sin_port = htons(DISCOVERY_MULTICAST_PORT);
  if (inet_pton(AF_INET, DISCOVERY_MULTICAST_IP, &discovery.sin_addr) != 1) {
    fprintf(stderr, "netif_wasm: invalid discovery multicast address\n");
    goto done;
  }

  /* UDP connect selects the outgoing route but does not send a packet. */
  if (LWIP_NETIF_CONNECT(socket_fd, (const struct sockaddr *)&discovery,
                         sizeof(discovery)) != 0) {
    fprintf(stderr, "netif_wasm: connect() failed: errno=%d\n", errno);
    goto done;
  }

  if (LWIP_NETIF_GETSOCKNAME(socket_fd, (struct sockaddr *)&local,
                             &local_length) != 0 ||
      local.sin_family != AF_INET || local.sin_addr.s_addr == INADDR_ANY) {
    fprintf(stderr,
            "netif_wasm: getsockname() did not return a local IPv4 address\n");
    goto done;
  }

  *local_ip = local.sin_addr.s_addr;
  result = 0;

done:
  LWIP_NETIF_CLOSE(socket_fd);
  return result;
}

int
netif_wasm_add(const char* ipaddr, const char* netmask)
{
  in_addr_t addr;
  in_addr_t mask;
  (void)ipaddr; /* Kept only for compatibility with existing applications. */

  if (netmask == NULL || inet_pton(AF_INET, netmask, &mask) != 1) {
    fprintf(stderr, "netif_wasm: invalid netmask\n");
    return NETIF_WASM_REFRESH_FAILED;
  }
  /* The statically initialized core mutex is available before MX_LWIP_Init.
   * Take it before the startup probe so all publication uses the same guard. */
  sys_lock_tcpip_core();
  if (probe_local_ip(&addr) != 0) {
    sys_unlock_tcpip_core();
    return NETIF_WASM_REFRESH_FAILED;
  }
  netif_wasm.netmask.addr = mask;
  netif_wasm.ip_addr.addr = addr;
  netif_default = &netif_wasm;
  sys_unlock_tcpip_core();
  printf("netif_wasm: dynamic ip=0x%08x mask=0x%08x\n", addr, mask);
  return 0;
}

int
netif_wasm_refresh(void)
{
  in_addr_t addr;
  int lock_result = sys_trylock_tcpip_core();

  if (lock_result == EBUSY) {
    return NETIF_WASM_REFRESH_BUSY;
  }
  if (lock_result != 0) {
    return NETIF_WASM_REFRESH_FAILED;
  }

  /* The existing default netif is the validity authority. Refresh before
   * startup is not initialization: only netif_wasm_add publishes it. */
  if (netif_default == NULL) {
    sys_unlock_tcpip_core();
    return NETIF_WASM_REFRESH_FAILED;
  }

  /* Owning the shared core mutex before probing guarantees at most one active
   * refresh probe and makes publication atomic with all selected core readers. */
  if (probe_local_ip(&addr) != 0) {
    sys_unlock_tcpip_core();
    return NETIF_WASM_REFRESH_FAILED;
  }

  if (netif_default->ip_addr.addr == addr) {
    sys_unlock_tcpip_core();
    return NETIF_WASM_REFRESH_UNCHANGED;
  }

  netif_default->ip_addr.addr = addr;
  sys_unlock_tcpip_core();
  printf("netif_wasm: local ip changed to 0x%08x\n", addr);
  return NETIF_WASM_REFRESH_CHANGED;
}

void
netif_wasm_get_ip_snapshot_core_locked(netif_wasm_ip_snapshot_t *snapshot)
{
  if (snapshot == NULL) {
    return;
  }
  snapshot->valid = netif_default != NULL;
  snapshot->ip_addr = snapshot->valid ? netif_default->ip_addr.addr : 0;
  snapshot->netmask = snapshot->valid ? netif_default->netmask.addr : 0;
}

int
netif_wasm_get_ip_snapshot(netif_wasm_ip_snapshot_t *snapshot)
{
  if (snapshot == NULL) {
    return -1;
  }
  sys_lock_tcpip_core();
  netif_wasm_get_ip_snapshot_core_locked(snapshot);
  sys_unlock_tcpip_core();
  return 0;
}

struct netif* netif_wasm_get(uint32_t ipaddr)
{
  (void)ipaddr;
  /* Internal lwIP bind/IGMP callers must hold the core mutex. */
  return netif_default;
}
