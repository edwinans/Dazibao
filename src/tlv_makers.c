#include <string.h>
#include <stdio.h>

#include "../include/tlv_makers.h"

int tlv_pad1(u_int8_t **buffer){
    if ((*buffer = malloc(1)) == NULL)
        return -1;

    **buffer = TYPE_PAD1;

    return 1;
}

int tlv_padn(u_int8_t **buffer, u_int8_t n){
    int size = HEADER_OFFSET + n;
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_PADN;
    (*buffer)[1] = n;
    memset(*buffer + HEADER_OFFSET, 0, n);

    return size;
}

int tlv_neighbour_req(u_int8_t **buffer){
    if ((*buffer = malloc(HEADER_OFFSET)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_NEIGHBOUR_REQ;
    (*buffer)[1] = 0;
    
    return HEADER_OFFSET;
}

int tlv_neighbour(u_int8_t **buffer, const struct in6_addr *addr, u_int16_t port){
    int size = HEADER_OFFSET + sizeof(addr) + sizeof(port);
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_NEIGHBOUR;
    (*buffer)[1] = size - HEADER_OFFSET;
    memcpy(*buffer + HEADER_OFFSET, addr, sizeof(struct in6_addr));
    memcpy(*buffer + HEADER_OFFSET + sizeof(addr), &port, sizeof(port));
    
    return size;
}

int tlv_net_hash(u_int8_t **buffer, const char *hash){
    int size = HEADER_OFFSET + HASH_SIZE;
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_NET_HASH;
    (*buffer)[1] = HASH_SIZE;
    memcpy(*buffer + HEADER_OFFSET, hash, HASH_SIZE);
    
    return size;
}

int tlv_netstate_req(u_int8_t **buffer){
    if ((*buffer = malloc(HEADER_OFFSET)) == NULL)
    return -1;

    (*buffer)[0] = TYPE_NETSTATE_REQ;
    (*buffer)[1] = 0;
    
    return HEADER_OFFSET;
}

int tlv_node_hash(u_int8_t **buffer, node_id id, seq_n seqno, const char *hash){
    int size = HEADER_OFFSET + sizeof(node_id) + sizeof(seqno) + HASH_SIZE;
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_NODE_HASH;
    (*buffer)[1] = size - HEADER_OFFSET;

    u_int8_t *offset = *buffer + HEADER_OFFSET;
    memcpy(offset, &id, sizeof(node_id));
    offset += sizeof(node_id);
    memcpy(offset, &seqno, sizeof(seq_n));
    offset += sizeof(seq_n);
    memcpy(offset, hash, HASH_SIZE);

    return size;
}

int tlv_nodestate_req(u_int8_t **buffer, node_id id){
    int size = HEADER_OFFSET + sizeof(node_id);
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_NODESTATE_REQ;
    (*buffer)[1] = sizeof(node_id);
    memcpy(*buffer + HEADER_OFFSET, &id, sizeof(node_id));
    
    return size;
}

int tlv_nodestate(u_int8_t **buffer, node_id id, seq_n seqno, const char *hash, const char *data, const u_int8_t datalen){
    if(datalen>192)
        return -1;
    int size = HEADER_OFFSET + sizeof(node_id) + sizeof(seqno) + HASH_SIZE + datalen;
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_NODESTATE;
    (*buffer)[1] = size - HEADER_OFFSET;

    u_int8_t *offset = *buffer + HEADER_OFFSET;
    memcpy(offset, &id, sizeof(node_id));
    offset += sizeof(node_id);
    memcpy(offset, &seqno, sizeof(seq_n));
    offset += sizeof(seq_n);
    memcpy(offset, hash, HASH_SIZE);
    offset += HASH_SIZE;
    memcpy(offset, data, datalen);

    return size;
}

int tlv_warning(u_int8_t **buffer, const char *msg, u_int8_t msglen){
    int size = HEADER_OFFSET + msglen;
    if ((*buffer = malloc(size)) == NULL)
        return -1;

    (*buffer)[0] = TYPE_WARNING;
    (*buffer)[1] = msglen;
    memcpy(*buffer + HEADER_OFFSET, msg, msglen);

    return size;
}