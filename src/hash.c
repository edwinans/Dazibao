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
    size_t contentlen = sizeof(node_id) + sizeof(seq_n) + node->data_len;
    uint8_t *content = malloc(contentlen);
    seq_n hseqno = htons(node->seqno);
    //node_id hnode_id = htobe64(node->id);

    if(content == NULL)
        return -1;

    uint8_t *ptr = content;
    memcpy(ptr, &node->id, sizeof(node_id));
    ptr += sizeof(node_id);
    memcpy(ptr, &hseqno, sizeof(seq_n));
    ptr += sizeof(seq_n);
    memcpy(ptr, node->data, node->data_len);

    if(SHA128(hash, content, contentlen) < 0)
        return -1;
    
    print_hash128(hash);
    free(content);
    
    return 0;
}

//H = h(concat of nodes)
int net_hash(uint8_t *hash, node_t nodes[], size_t nb, size_t size){
    size_t contentlen = HASH_SIZE * nb;
    uint8_t *content = malloc(contentlen);
    if(content == NULL)
        return -1;

    for(int i = 0; i<nb; i++){
        if(node_hash(hash, nodes+i) < 0)
            return -1;
        memcpy(content + 2*i, hash, HASH_SIZE);
    }
    
    if(SHA128(hash, content, contentlen) < 0)
        return -1;
    
    free(content);
    
    return 0;
}

bool hash_equals(uint8_t *hash1, uint8_t *hash2){
    return !memcmp(hash1, hash2, HASH_SIZE);
}

void print_hash128(uint8_t *hash){
    for(int x = 0; x < HASH_SIZE ; x++)
        printf("%02x", hash[x]);
    putchar('\n');
}