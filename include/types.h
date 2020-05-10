#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <endian.h>

#define MAGIC 95
#define VERSION 1
#define HASH_SIZE 16
#define INIT_NODES_LEN 100
#define MAX_NBR 15
#define MAX_DATA_LEN 195

typedef uint64_t node_id;
typedef uint16_t seq_n;


typedef struct node {
    node_id id; //0 if not present, else >0
    seq_n seqno;
    size_t data_len;
    uint8_t data[MAX_DATA_LEN];
} node_t;

typedef struct neighbour {
    bool is_permanent;
    time_t last_data;
    struct sockaddr_in6 *addr;
} neighbour_t;

typedef struct pair {
    node_id id;
    size_t nb_neighbours;
    neighbour_t neighbours[MAX_NBR];
    size_t nb_nodes;
    size_t nodes_len;
    node_t *nodes;
} pair_t;

typedef struct tlv {
    size_t size;
    uint8_t *content;
    struct tlv *next;
} tlv_t;

typedef struct packet{
    uint8_t magic;
    uint8_t version;
    uint16_t body_len;
    tlv_t *first_tlv;
} packet_t;


#endif