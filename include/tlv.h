#ifndef TLV_H
#define TLV_H

#include <netinet/in.h>
#include "types.h"


#define HEADER_OFFSET 2

#define TYPE_PAD1 0
#define TYPE_PADN 1
#define TYPE_NEIGHBOUR_REQ 2
#define TYPE_NEIGHBOUR 3
#define TYPE_NET_HASH 4
#define TYPE_NETSTATE_REQ 5
#define TYPE_NODE_HASH 6
#define TYPE_NODESTATE_REQ 7
#define TYPE_NODESTATE 8
#define TYPE_WARNING 9


extern int tlv_pad1(u_int8_t **buffer);
extern int tlv_padn(u_int8_t **buffer, u_int8_t n);
extern int tlv_neighbour_req(u_int8_t **buffer);
extern int tlv_neighbour(u_int8_t **buffer, const struct in6_addr*, u_int16_t port);
extern int tlv_net_hash(u_int8_t **buffer, const char *hash);
extern int tlv_netstate_req(u_int8_t **buffer);
extern int tlv_node_hash(u_int8_t **buffer, node_id id, seq_n seqno, const char *hash);
extern int tlv_nodestate_req(u_int8_t **buffer, node_id id);
extern int tlv_nodestate(u_int8_t **buffer, node_id id, seq_n seqno, const char *hash, const char *data, const u_int8_t datalen);
extern int tlv_warning(u_int8_t **buffer, const char *msg, const u_int8_t msglen);

#endif