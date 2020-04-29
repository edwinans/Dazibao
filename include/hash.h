#ifndef HASH_H
#define HASH_H

#include <stdlib.h>
#include <stdio.h>
#include "sha.h"
#include "types.h"

int SHA128(uint8_t *hash, const uint8_t *content, const uint8_t contentlen);
void print_hash128(uint8_t *hash);

#endif