#include "../include/hash.h"


int SHA128(uint8_t *hash, const uint8_t *content, const uint8_t contentlen){
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

void print_hash128(uint8_t *hash){
    for(int x = 0; x < HASH_SIZE ; x++)
        printf("%02x", hash[x]);
    putchar('\n');
}