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

int saddr6_cmp(struct sockaddr_in6* p1, struct sockaddr_in6* p2){
    char s1[INET6_ADDRSTRLEN], s2[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(p1->sin6_addr), s1, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &(p2->sin6_addr), s2, INET6_ADDRSTRLEN);

    return (!strcmp(s1, s2) && (p1->sin6_port == p2->sin6_port));
}

int init_pair(pair_t **my_pair){
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

    return 0;
}

int init_neighbours(pair_t *pair, const char boot_host[], const char boot_port[]){

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
    char s[INET6_ADDRSTRLEN];
    for(struct addrinfo *p = peer_address; p != NULL; p = p->ai_next) {
        struct sockaddr_in6 *saddr6 = (struct sockaddr_in6*) p->ai_addr;
        inet_ntop(AF_INET6, &(saddr6->sin6_addr), s, INET6_ADDRSTRLEN);
        printf("permanent addr: %s | port : %d | len: %d\n", s, saddr6->sin6_port, p->ai_addrlen);

        pair->neighbours[i].addr = saddr6;
        pair->neighbours[i].is_permanent = 1;
        pair->neighbours[i].last_data = time(0);
        i++;
    }
    pair->nb_neighbours = i;

    freeaddrinfo(peer_address);

    return 0;
}

int main(int argc, char const *argv[]){
    pair_t *my_pair;
    if(init_pair(&my_pair) < 0){
        printf("init_pair error\n");
        return 0;
    }

    if(init_neighbours(my_pair, JCH_HOST, UDP_DEF_PORT) <0){
        printf("init_neighbours error\n");
        return 0;
    }
    
    return 0;
}
