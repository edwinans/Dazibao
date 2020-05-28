#ifndef HASH_H
#define HASH_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "sha.h"
#include "types.h"

int SHA128(uint8_t *hash, const uint8_t *content, const size_t contentlen);
void print_hash128(uint8_t *hash);
int node_hash(uint8_t *hash, node_t *node); //h
bool hash_equals(uint8_t *hash1, uint8_t *hash2);
int net_hash(uint8_t *hash, node_t nodes[],size_t nb, size_t size); //H

#endif