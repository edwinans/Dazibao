#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "tlv.h"

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
    memcpy(*buffer + HEADER_OFFSET, addr, sizeof(addr));
    memcpy(*buffer + HEADER_OFFSET + sizeof(addr), &port, sizeof(port));
    
    return size;
}