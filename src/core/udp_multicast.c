#include "udp_multicast.h"
#include "netif_wasm_add.h"
#include "sys_utils.h"
#include <errno.h>
#include <string.h>


int udp_mc_initialize(UdpMcInfoType* info)
{

  info->sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (info->sd < 0) {
    CMSIS_IMPL_ERROR("udp_mc_initialize() socket() err=%d", errno);
    return -1;
  }
  int set_option_on = 1;

  int res = setsockopt(info->sd, SOL_SOCKET, SO_REUSEADDR, (char*)&set_option_on,
                       sizeof(set_option_on));
  if (res < 0) {
    CMSIS_IMPL_ERROR("udp_mc_initialize() setsockopt() err=%d", errno);
    return -1;
  }
  return 0;
}

int udp_mc_recover(UdpMcInfoType* info)
{
  struct sockaddr_in local_addr;
  uint32_t groupaddr = info->multicast_request.imr_multiaddr.s_addr;

  if (netif_wasm_refresh() < 0 || netif_default == NULL) {
    return -1;
  }
  info->local_ipaddr.addr = netif_default->ip_addr.addr;

  if (info->sd >= 0) {
    close(info->sd);
    info->sd = -1;
  }

  if (udp_mc_initialize(info) != 0) {
    return -1;
  }

  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = htons(info->port);
  if (bind(info->sd, (struct sockaddr*)&local_addr, sizeof(local_addr)) != 0) {
    close(info->sd);
    info->sd = -1;
    return -1;
  }

  if (groupaddr != 0 && udp_mc_bind_addr(info, groupaddr) != 0) {
    close(info->sd);
    info->sd = -1;
    return -1;
  }

  return 0;
}

static int udp_mc_enable(UdpMcInfoType* info)
{
  int rc = 0;
  in_addr_t ipaddr;

  ipaddr = info->local_ipaddr.addr;

  //set eth multicast mode
  // rc = setsockopt(info->sd, IPPROTO_IP, IP_MULTICAST_IF,
  //                 (char*)&ipaddr, sizeof(ipaddr));
  if (rc != 0) {
    CMSIS_IMPL_ERROR("udp_mc_enable() setsockopt(IP_MULTICAST_IF) err=%d", errno);
    return -1;
  }
#if 0 /* not used loop callback */
  u_char loop = 0;
  rc = setsockopt(info->sd, IPPROTO_IP, IP_MULTICAST_LOOP,
                  (char*)&loop, sizeof(loop));
  if (rc != 0) {
    CMSIS_IMPL_ERROR("udp_mc_enable() setsockopt(IP_MULTICAST_IF) err=%d", errno);
    return -1;
  }
#endif
  //set TTL
  printf("set TTL: ttl=%d, optlen=%d\n", info->ttl, sizeof(info->ttl));
  rc = setsockopt(info->sd, IPPROTO_IP, IP_MULTICAST_TTL,
                  (void*)&(info->ttl), sizeof(info->ttl));
  if (rc != 0) {
    CMSIS_IMPL_ERROR("udp_mc_enable() setsockopt(TTL) err=%d", errno);
    return -1;
  }

  return 0;
}

int udp_mc_bind_addr(UdpMcInfoType* info, uint32_t groupaddr)
{
  int err = udp_mc_enable(info);
  if (err != 0) {
    CMSIS_IMPL_INFO("udp_mc_bind_addr(): udp_mc_enable() err=%d", err);
    return -1;
  }

  //bind multicast addr on local ipaddr
  info->multicast_request.imr_multiaddr.s_addr = groupaddr;
  info->multicast_request.imr_interface.s_addr = info->local_ipaddr.addr;
  int rc = setsockopt(info->sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                      (void*)&info->multicast_request, sizeof(struct ip_mreq));
  if (rc < 0) {
    CMSIS_IMPL_ERROR("can not set multicast setting: errno=%d", errno);
    return -1;
  }
  return 0;
}
