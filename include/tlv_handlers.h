#ifndef TLV_HANDLERS_H
#define TLV_HANDLERS_H

#include <netinet/in.h>
#include "network.h"
#include "types.h"
#include "tlv_makers.h"


int handle_tlv_pad1(uint8_t *tlv);
int handle_tlv_padn(uint8_t *tlv);
int handle_tlv_neighbour_req(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s );
int handle_tlv_neighbour(pair_t *pair, uint8_t *tlv, uint16_t size, SOCKET s);
int handle_tlv_nethash(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s);
int handle_tlv_netstate_req(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s);
int handle_tlv_node_hash(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s);
int handle_tlv_nodesate_req(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s);
int handle_tlv_nodestate(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s);
int handle_tlv_warning(u_int8_t *tlv);

#endif // !TLV_HANDLERS