#include "../include/hash.h"


int SHA128(uint8_t *hash, const uint8_t *content, const size_t contentlen){
    SHA256Context ctx;

    int rc = SHA256Reset(&ctx);
    if(rc < 0)
       return -1;
    rc = SHA256Input(&ctx, content, contentlen);
    if(rc < 0)
       return -1;
    rc = SHA256Result(&ctx, hash);
    if(rc != 0)
        return -1;
    return 0;
}

//h_i = h(node) = h(i, s, d)
int node_hash(uint8_t *hash, node_t *node){
    size_t contentlen = sizeof(node_id) + sizeof(seq_n) + sizeof(node->data_len);
    uint8_t *content = malloc(contentlen);
    uint16_t gros = htons(node->seqno);

    if(content == NULL)
        return -1;

    uint8_t *ptr = content;
    memcpy(ptr, &node->id, sizeof(node->id));
    ptr += sizeof(node->id);
    memcpy(ptr, &gros, sizeof(gros));
    ptr += sizeof(gros);
    memcpy(ptr, node->data, node->data_len);

    if(SHA128(hash, content, contentlen) < 0)
        return -1;
    
    free(content);

    return 0;
}

//H = h(concat of nodes)
int net_hash(uint8_t *hash, node_t nodes[], size_t nb, size_t size){
    size_t contentlen = HASH_SIZE * nb;
    uint8_t *content = malloc(contentlen);
    if(content == NULL)
        return -1;

    int act_nb = 0;
    for(int i = 0; i<size; i++){
        if(nodes[i].id){
            if(nodes[i].id != i)
                return -1;
            if(node_hash(hash, nodes+i) < 0)
                return -1;
            memcpy(content + 2*act_nb, hash, HASH_SIZE);
            act_nb++;
        }
    }
    if(act_nb != nb)
        return -1;
    
    if(SHA128(hash, content, contentlen) < 0)
        return -1;
    
    free(content);
    
    return 0;
}

void print_hash128(uint8_t *hash){
    for(int x = 0; x < HASH_SIZE ; x++)
        printf("%02x", hash[x]);
    putchar('\n');
}