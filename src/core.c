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

int send_packet_to(uint8_t *tlv, uint16_t size, neighbour_t *nbr, SOCKET s){
    uint8_t *packet = malloc(size + 4);
    if(packet == NULL)
        return -1;
    packet[0] = MAGIC;
    packet[1] = VERSION;
    int hsize = htons(size);
    memcpy(packet + 2, &hsize, 2);
    memcpy(packet + 4, tlv, size);

    int rc = sendto(s, packet, size + 4, 0, (struct sockaddr*) nbr->addr, sizeof(struct sockaddr_in6));
    if(rc < 0 || rc>PMTU)
        return -1;
        
    return rc;
}

int saddr6_equals(struct sockaddr_in6* p1, struct sockaddr_in6* p2){
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

int add_neighbour(pair_t *pair, struct sockaddr_in6* addr){
    for(int i=0; i< pair->nb_neighbours; i++){
        neighbour_t nbr = pair->neighbours[i];
        if(saddr6_equals(addr, nbr.addr)){
            nbr.last_data = time(0);
            return 0;
        }
    }

    if(pair->nb_neighbours >= MAX_NBR){
        printf("limit of nbrs (%d) reached ; can not add new neighbour\n", MAX_NBR);
        return -1;
    }

    neighbour_t new_nbr = {0, time(0), addr};
    pair->neighbours[++pair->nb_neighbours] = new_nbr;

    return 0;
}

int update_neighbours(pair_t *pair){
    time_t now = time(0);
    int init_nb = pair->nb_neighbours;

    for(int i=0; i< pair->nb_neighbours; i++){
        neighbour_t nbr = pair->neighbours[i];
        if(!nbr.is_permanent && (now - nbr.last_data >70)){
            memmove(pair->neighbours + 1, pair->neighbours + i + 1,
                (pair->nb_neighbours - i - 1) * sizeof(neighbour_t));
            pair->nb_neighbours--;
            i--;
        }
    } 

    return init_nb - pair->nb_neighbours;
}

int explore_neighbours(pair_t *pair, SOCKET s){
    if(pair->nb_neighbours >=5)
        return 0;
    
    srand(time(0));
    neighbour_t rand_nbr = pair->neighbours[rand() % pair->nb_neighbours];
    uint8_t *tlv;
    int size;
    if( (size = tlv_neighbour_req(&tlv)) < 0)
        return -1;
    
    int packet_size;
    if( (packet_size = send_packet_to(tlv, size, &rand_nbr, s)) < 0)
        return -1;
    
    return packet_size;
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

    SOCKET socket_peer;
    socket_peer = socket(AF_INET6, SOCK_DGRAM, 0);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    explore_neighbours(my_pair, socket_peer);


    
    free(my_pair->nodes);
    free(my_pair);
    CLOSESOCKET(socket_peer);

    return 0;
}
