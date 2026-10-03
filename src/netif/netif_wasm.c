#include "netif_wasm_add.h"
#include "netif_wasm_get.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wasi_socket_ext.h>

#define DISCOVERY_MULTICAST_IP "239.255.0.1"
#define DISCOVERY_MULTICAST_PORT 7400
#define DEFAULT_NETMASK "255.255.255.0"

static struct netif netif_wasm;
struct netif* netif_default = NULL;
static int ip_changed_pending = 0;

static int
probe_local_ip(in_addr_t *local_ip)
{
  int socket_fd = -1;
  struct sockaddr_in discovery = {0};
  struct sockaddr_in local = {0};
  socklen_t local_length = sizeof(local);

  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "netif_wasm: socket() failed: errno=%d\n", errno);
    return -1;
  }

  discovery.sin_family = AF_INET;
  discovery.sin_port = htons(DISCOVERY_MULTICAST_PORT);
  if (inet_pton(AF_INET, DISCOVERY_MULTICAST_IP, &discovery.sin_addr) != 1) {
    fprintf(stderr, "netif_wasm: invalid discovery multicast address\n");
    close(socket_fd);
    return -1;
  }

  /* UDP connect selects the outgoing route but does not send a packet. */
  if (connect(socket_fd, (const struct sockaddr *)&discovery,
              sizeof(discovery)) != 0) {
    fprintf(stderr, "netif_wasm: connect() failed: errno=%d\n", errno);
    close(socket_fd);
    return -1;
  }

  if (getsockname(socket_fd, (struct sockaddr *)&local, &local_length) != 0 ||
      local.sin_family != AF_INET || local.sin_addr.s_addr == INADDR_ANY) {
    fprintf(stderr,
            "netif_wasm: getsockname() did not return a local IPv4 address\n");
    close(socket_fd);
    return -1;
  }

  *local_ip = local.sin_addr.s_addr;
  close(socket_fd);
  return 0;
}

int
netif_wasm_add(const char* ipaddr, const char* netmask)
{
  in_addr_t addr;
  in_addr_t mask;
  (void)ipaddr; /* Kept only for compatibility with existing applications. */

  if (netmask == NULL || inet_pton(AF_INET, netmask, &mask) != 1) {
    fprintf(stderr, "netif_wasm: invalid netmask\n");
    return -1;
  }
  if (probe_local_ip(&addr) != 0) {
    return -1;
  }

  netif_wasm.ip_addr.addr = addr;
  netif_wasm.netmask.addr = mask;
  netif_default = &netif_wasm;
  __atomic_store_n(&ip_changed_pending, 0, __ATOMIC_SEQ_CST);
  printf("netif_wasm: dynamic ip=0x%08x mask=0x%08x\n", addr, mask);
  return 0;
}

int
netif_wasm_refresh(void)
{
  in_addr_t addr;
  in_addr_t mask;
  int changed;

  if (probe_local_ip(&addr) != 0) {
    return -1;
  }

  /* Native globals are recreated from scratch by WAMR restore. Recreate the
   * netif object before refreshing its address in that case. */
  if (netif_default == NULL) {
    if (inet_pton(AF_INET, DEFAULT_NETMASK, &mask) != 1) {
      return -1;
    }
    netif_wasm.netmask.addr = mask;
    netif_wasm.ip_addr.addr = addr;
    netif_default = &netif_wasm;
    __atomic_store_n(&ip_changed_pending, 1, __ATOMIC_SEQ_CST);
    printf("netif_wasm: local ip initialized after restore to 0x%08x\n",
           addr);
    return 1;
  }

  changed = netif_wasm.ip_addr.addr != addr;
  netif_wasm.ip_addr.addr = addr;
  if (changed) {
    __atomic_store_n(&ip_changed_pending, 1, __ATOMIC_SEQ_CST);
    printf("netif_wasm: local ip changed to 0x%08x\n", addr);
  }
  return changed ? 1 : 0;
}

int
netif_wasm_take_ip_changed(void)
{
  return __atomic_exchange_n(&ip_changed_pending, 0, __ATOMIC_SEQ_CST);
}

struct netif* netif_wasm_get(uint32_t ipaddr)
{
  return netif_default;
}
