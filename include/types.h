#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define HASH_SIZE 16
#define MAGIC 95
#define VERSION 1
#define MAX_NBR 15

typedef const uint64_t node_id;
typedef int16_t seq_n;


typedef struct node {
    node_id id;
    seq_n seqno;
    size_t size;
    uint8_t data[];
} node_t;

typedef struct neighbour {
    bool is_permanent;
    time_t last_data;
    struct sockaddr_in6 *addr;
} neighbour_t;

typedef struct pair {
    node_id id;
    node_t node;
    neighbour_t neighbours[MAX_NBR];
    node_t pub_data[100];
} pair_t;

typedef struct tlv {
    size_t size;
    uint8_t *content;
    struct tlv *next;
} tlv_t;

typedef struct packet{
    uint8_t magic;
    uint8_t version;
    uint16_t body_length;
    tlv_t *first_tlv;
    node_t *dst;
} packet_t;



#endif