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
#include <unistd.h>
#include <fcntl.h>

#include "../include/network.h"
#include "../include/hash.h"
#include "../include/tlv_makers.h"

bool debug = 0;

void print_addr(struct sockaddr_in6 *saddr6){
    char s[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(saddr6->sin6_addr), s, INET6_ADDRSTRLEN);
    printf("addr6: %s | port : %d \n", s, ntohs(saddr6->sin6_port));
}

int checkpkt_hd(uint8_t *packet, uint16_t size){
    uint8_t *itr = packet;
    if(*itr != MAGIC)
        return -1;
    itr++;
    if(*itr != VERSION)
        return -1;
    itr++;
    uint16_t body_len = ntohs(*((uint16_t*) itr));
    if(debug){
        printf("chckpkt ; body-len :%d , size : %d\n", body_len, size);
    }
    
    if(size > body_len + 4)
        return -2;

    return 0;
}

int send_packet_to(uint8_t *tlv, uint16_t size, neighbour_t *nbr, SOCKET s){
    uint8_t packet[size+4];
    if(packet == NULL)
        return -1;
    packet[0] = MAGIC;
    packet[1] = VERSION;
    int hsize = htons(size);
    memcpy(packet + 2, &hsize, 2);
    memcpy(packet + 4, tlv, size);

    if(checkpkt_hd(packet, size+4) < 0)
        return -1; 
    if(debug){
        printf("sending packet to nbr ->");
        print_addr(nbr->addr);
    }
    int rc = sendto(s, packet, sizeof(packet), 0, (struct sockaddr*) nbr->addr, sizeof(struct sockaddr_in6));
    if(rc < 0 || rc>PMTU){
        fprintf(stderr, "error sending packet , nb_bytes: %d ; %s\n", rc,strerror( errno ));
        return -1;
    }
        

    return rc;
}

int send_pad1_to(neighbour_t *nbr, SOCKET s){
    uint8_t *pad1_tlv;
    int sz = tlv_pad1(&pad1_tlv);
    if(send_packet_to(pad1_tlv, sz, nbr, s) < 0){
        free(pad1_tlv);
        return -1;
    }
    
    free(pad1_tlv);
    return 0;
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
    for(struct addrinfo *p = peer_address; p != NULL; p = p->ai_next) {
        struct sockaddr_in6 *saddr6 = (struct sockaddr_in6*) p->ai_addr;
        printf("permanent nbr added: ");
        print_addr(saddr6);
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

    //return nb of deleted members
    return init_nb - pair->nb_neighbours;
}

int explore_neighbours(pair_t *pair, SOCKET s){
    if(pair->nb_neighbours >= 5)
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
    
    free(tlv);

    return packet_size;
}

int handle_tlv_neighbour(pair_t *pair, uint8_t *tlv, uint16_t size, SOCKET s){
    uint8_t *offset = tlv;
    uint8_t type = *offset;
    if(type != TYPE_NEIGHBOUR){
        printf("bad tlv type ; NEIGHBOUR\n");
        return -1;
    }
    offset++;

    struct sockaddr_in6 *addr = malloc(sizeof(struct sockaddr_in6));
    if(addr == NULL)
        return -1;

    size_t tlv_len = *offset;
    offset++;

    size_t true_tlv_len = sizeof(addr->sin6_addr) + sizeof(addr->sin6_port);
    if(tlv_len != true_tlv_len){
        printf("bad tlv_neighbour len %ld != 18", tlv_len);
        return -1;
    }

    memset(addr, 0, sizeof(struct sockaddr_in6));
    addr->sin6_family = AF_INET6;
    memcpy(&addr->sin6_addr, offset, sizeof(addr->sin6_addr));
    offset += sizeof(addr->sin6_addr);

    in_port_t hport = ntohs(*offset);
    memcpy(&addr->sin6_port, &hport, sizeof(addr->sin6_port));

    neighbour_t dst_nbr = {0, 0, addr};

    uint8_t *nethash_tlv;
    uint8_t hash[HASH_SIZE*2];
    if(net_hash(hash, pair->nodes, pair->nb_nodes, pair->nodes_len) < 0){
        printf("bad network_hash in handle_tlv_neighbour function\n");
        return -1;
    }
    int nethash_size = tlv_net_hash(&nethash_tlv, hash);
    if(nethash_size < 0){
        printf("malloc failed in tlv_net_hash ; handle_tlv_neighbour\n");
        return -1;
    }

    if(send_packet_to(nethash_tlv, nethash_size, &dst_nbr, s) < 0)
        return -1;
    

    free(addr);
    free(nethash_tlv);

    return 0;
}

int handle_tlv_nethash(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s){
    uint8_t my_hash[HASH_SIZE*2];
    if(net_hash(my_hash, pair->nodes, pair->nb_nodes, pair->nodes_len) < 0){
        printf("net_hash() failed in handle_tlv_nethash() \n");
        return -1;
    }

    uint8_t *offset = tlv;
    uint8_t type = *offset;
    if(type != TYPE_NET_HASH){
        printf("bad tlv type ; NET_HASH\n");
        return -1;
    }
    offset++;

    uint8_t tlv_len = *offset;
    offset++;

    uint8_t true_tlv_len = HASH_SIZE;
    if(tlv_len != true_tlv_len){
        printf("bad tlv_neighbour len %d != %d", tlv_len, true_tlv_len);
        return -1;
    }

    uint8_t recvd_hash[HASH_SIZE];
    memcpy(recvd_hash, offset, HASH_SIZE);

    if(hash_equals(my_hash, recvd_hash)){
        if(debug)
            printf("same network hash recieved\n");
    }
    else{
        uint8_t *netreq_tlv;
        int netreq_size = tlv_netstate_req(&netreq_tlv);
        if(netreq_size < 0){
            printf("malloc() failed in tlv_netstate_req ; handle_tlv_nethash \n");
            return -1;
        }
        neighbour_t dst_nbr = {0, 0, src_addr};

        send_packet_to(netreq_tlv, netreq_size, &dst_nbr, s);

        free(netreq_tlv);
    }

    return 0;
}

int flood_net_hash(pair_t *pair, SOCKET s){
    uint8_t *nethash_tlv;
    uint8_t hash[HASH_SIZE*2];
    if(net_hash(hash, pair->nodes, pair->nb_nodes, pair->nodes_len) < 0){
        printf("bad network_hash in flood_net_hash function\n");
        return -1;
    }
    int nethash_size = tlv_net_hash(&nethash_tlv, hash);
    if(nethash_size < 0){
        printf("malloc() failed in tlv_net_hash() ; flood_net_hash()\n");
        return -1;
    }

    for(int i=0; i< pair->nb_neighbours; i++){
        send_packet_to(nethash_tlv, nethash_size, pair->neighbours + i, s);
    }

    return pair->nb_neighbours;
}

int main(int argc, char const *argv[]){
    if(argc>1 && !memcmp(argv[1], "-d", 2))  debug=1;

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
    int rc = fcntl(socket_peer, F_GETFL);
    if(rc < 0)
        perror("socket in core :");
    rc = fcntl(socket_peer, F_SETFL, rc | O_NONBLOCK);
    if(rc < 0)
        perror("socket in core :");

    uint8_t response[PMTU];

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_peer, &master);
    SOCKET max_socket = socket_peer;

    /* handle_nethash test : 
    send_pad1_to(my_pair->neighbours, socket_peer);

    struct sockaddr_storage sender_addr;
    socklen_t client_len = sizeof(sender_addr);
    rc = recvfrom(socket_peer,
            response, PMTU,
            0,
            (struct sockaddr*) &sender_addr, &client_len);

    printf("Received (%d bytes)\n",rc); 

    handle_tlv_nethash(my_pair, response +4, (struct sockaddr_in6 *)&sender_addr, socket_peer); test*/


    // while(1){

    //     explore_neighbours(my_pair, socket_peer);

    //     fd_set reads;
    //     reads = master;
    //     struct timeval to = {20, 0};
    //     if (select(max_socket+1, &reads, 0, 0, &to) < 0) {
    //         fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
    //         return 1;
    //     }

    //     if(FD_ISSET(socket_peer, &reads)){
    //         rc = recvfrom(socket_peer, response, PMTU, 0, NULL, NULL);
    //         printf("Response %d bytes: \n", rc);

    //         if(checkpkt_hd(response, rc) < 0){
    //             printf("Bad packet header\n");
    //             continue;
    //         }
            
    //         if(handle_tlv_neighbour(my_pair, response + 4, rc - 4, socket_peer) < 0){
    //             printf("core error\n");
    //             break;
    //         }
    //     }
    // }
    
    free(my_pair->nodes);
    free(my_pair);
    CLOSESOCKET(socket_peer);

    return 0;
}
