/*
 * MIT License
 *
 * Copyright (c) 2020 Edwin Ansari - Yaniv Benichou
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/network.h"
#include "../include/hash.h"
#include "../include/tlv_makers.h"

int init_pair(pair_t **my_pair, const char boot_host[], const char boot_port[]){
    if ((*my_pair = malloc(sizeof(pair_t))) == NULL)
        return -1;
    pair_t *pair = *my_pair;

    srand(time(0));
    
    pair->id = rand();

    pair->nb_nodes = 1;
    pair->nodes_len = INIT_NODES_LEN;
    if((pair->nodes = malloc(sizeof(node_t) * INIT_NODES_LEN)) == NULL){
        free(pair);
        return -1;
    }
    (pair->nodes)[0] = (node_t){pair->id, 0, 0, {0}};


    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    struct addrinfo *peer_address;
    if (getaddrinfo(boot_host, boot_port, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return -1;
    }

    int i=0;
    for(struct addrinfo *p = peer_address; p != NULL; p = p->ai_next) {
        printf("addr: %d, len: %d\n", p->ai_addr->sa_family, p->ai_addrlen);
        pair->neighbours[i++].addr = (struct sockaddr_in6*) p->ai_addr;
    }
    pair->nb_neighbours = i;

    return 0;
}

int main(int argc, char const *argv[]){
    pair_t* my_pair;
    init_pair(&my_pair, JCH_HOST, UDP_DEF_PORT);

    
    return 0;
}
