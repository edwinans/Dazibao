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

int set_port(SOCKET s, in_port_t port){
    struct sockaddr_in6 local = {};
    local.sin6_family = AF_INET6;
    local.sin6_port = htons(port);


    printf("Binding socket to corresponding port (%d)...\n", port);
    if ( bind(s,(struct sockaddr*)&local, sizeof(local))) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return -1;
    }
    return 0;
}

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
        fprintf(stderr, "error sending packet , nb_bytes: %d ; %s\n", rc, strerror( errno ));
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
    uint8_t data[]= "Hello World!";
    (pair->nodes)[0] = (node_t){pair->id, 0, sizeof(data)-1, {0}};
    memcpy(&pair->nodes[0].data, data, sizeof(data)-1);

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
        if(debug)
            printf("limit of nbrs (%d) reached ; can not add new neighbour\n", MAX_NBR);
        return 0;
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


int handle_tlv_pad1(uint8_t *tlv){
    if(tlv[0] != TYPE_PAD1){
        printf("bad tlv type: PAD1 \n");
        return -1;
    }
    if(debug)
        printf("<- tlv_pad1 recvd \n");
    return 0;
}

int handle_tlv_padn(uint8_t *tlv){
    if(tlv[0] != TYPE_PADN){
        printf("bad tlv type: PADN \n");
        return -1;
    }
    uint8_t len = tlv[1];
    for(int i=0; i<len; i++){
        if(tlv[HEADER_OFFSET+i]){
            if(debug)
                printf("<- MBZ of TLV_PADN not correct! \n");
            return -1;
        }
    }
    if(debug)
        printf("<- tlv_padn recvd \n");

    return 0;
}

int handle_tlv_neighbour_req(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s ){
    uint8_t type = tlv[0];
    if(type != TYPE_NEIGHBOUR_REQ){
        printf("bad tlv type ;NEIGHBOUR_REQ \n");
        return -1;
    }
    uint8_t tlv_len = tlv[1];

    uint8_t true_tlv_len = 0;
    if(tlv_len != true_tlv_len){
        printf("bad tlv_neighbour_req len %d != %d", tlv_len, true_tlv_len);
        return -1;
    }

    srand(time(0));
    neighbour_t rand_nbr = pair->neighbours[rand() % pair->nb_neighbours];

    uint8_t *neighbour_tlv;
    int nbrtlv_size = tlv_neighbour(&neighbour_tlv,
        &rand_nbr.addr->sin6_addr,
        rand_nbr.addr->sin6_port);
    if(debug)
        printf("nrbtlv_size : %d \n", nbrtlv_size);
    if(nbrtlv_size < 0){
        printf("malloc() failed in tlv_neighbour() ; handle_tlv_neighbour_req \n");
        return -1;
    }
    neighbour_t dst_nbr = {0, 0, src_addr};

    send_packet_to(neighbour_tlv, nbrtlv_size, &dst_nbr, s);  

    free(neighbour_tlv);
    
    return 0;
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

int handle_tlv_netstate_req(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s){
    if(tlv[0] != TYPE_NETSTATE_REQ){
        printf("bad tlv type : NETSTATE_REQ \n");
        return -1;
    }
    if(tlv[1] != 0){
        printf("bad tlv len !=0 : NETSTATE_REQ \n");
        return -1;
    }

    size_t ntlv_size = HEADER_OFFSET + sizeof(node_id) + sizeof(seq_n) + HASH_SIZE;
    uint8_t node_hash_tlv[ntlv_size];
    uint8_t hash[HASH_SIZE*2];
    neighbour_t dst_nbr = {0, 0, src_addr};

    for(int i=0; i< pair->nb_nodes; i++){
        node_t node = pair->nodes[i];
        if(node_hash(hash, &node) < 0)
            printf("node_hash() failed in node_id : %ld \n", node.id);

        tlv_node_hash(node_hash_tlv, node.id, node.seqno, hash);

        if(debug)
            printf("try to send node_hash | node_id : %ld \n", node.id);

        send_packet_to(node_hash_tlv, ntlv_size, &dst_nbr, s);
    }

    return 0;
}

int handle_tlv_node_hash(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s){
    if(tlv[0] != TYPE_NODE_HASH){
        printf("bad tlv type : NODE_HASH \n");
        return -1;
    }
    uint8_t tlv_len = tlv[1];
    if(tlv_len != sizeof(node_id) + sizeof(seq_n) + HASH_SIZE){
        printf("incorrect tlv len: %u in handle_tlv_node_hash() \n", tlv_len);
        return -1;
    }
    uint8_t *offset = tlv + HEADER_OFFSET;
    
    node_id hid, rec_id;
    memcpy(&hid, offset, sizeof(node_id));
    rec_id = be64toh(hid);
    offset += sizeof(node_id);

    seq_n hseqno, rec_seqno; //todo
    memcpy(&hseqno, offset, sizeof(seq_n));
    rec_seqno = ntohs(hseqno);
    offset += sizeof(seq_n);

    uint8_t rec_hash[HASH_SIZE*2], my_hash[HASH_SIZE*2];
    memcpy(rec_hash, offset, HASH_SIZE);
    
    for(int i=0; i< pair->nb_nodes; i++){
        node_t node = pair->nodes[i];
        if(node.id == rec_id){
            if(node_hash(my_hash, &node) < 0)
                printf("node_hash() failed in handle_tlv_node_hash() \n");

            if(!hash_equals(my_hash, rec_hash)){
                if(debug)
                    printf("recvd node_hash not equals for node_id: %ld \n", node.id);
                
                break; //to send node_state_req
            }
            else{
                return 0; //nothing to do
            }
        }
    }

    //sending tlv_node_state_req ...
    uint8_t *nodestate_req_tlv;
    uint16_t nreq_tlv_size = tlv_nodestate_req(&nodestate_req_tlv, hid);
    if(nreq_tlv_size < 0){
        perror("malloc() failed in tlv_nodestate_req : handle_tlv_node_hash() \n");
        return -1;
    }
    neighbour_t dst_nbr = {0, 0, src_addr};
    send_packet_to(nodestate_req_tlv, nreq_tlv_size, &dst_nbr, s);

    free(nodestate_req_tlv);

    return 0;
}

int handle_tlv_nodesate_req(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s){
    // if(tlv[0] != TYPE_NODESTATE_REQ){
    //     printf("bad tlv type : NODE_STATE_REQ \n");
    //     return -1;
    // }
    uint8_t tlv_len = tlv[1];
    // if(tlv_len != sizeof(node_id)){
    //     printf("incorrect tlv len: %u in handle_tlv_nodestate_req() \n", tlv_len);
    //     return -1;
    // }

    node_id hid, rec_id;
    memcpy(&hid, tlv + 2, sizeof(node_id));
    rec_id = be64toh(hid);
    uint8_t hash[HASH_SIZE*2];

    uint8_t *nodestate_tlv;
    uint16_t nodestate_tlv_size;
    neighbour_t dst_nbr = {0, 0, src_addr};

    for(int i=0; i< pair->nb_nodes; i++){
        node_t node = pair->nodes[i];

        if(1){
            if(node_hash(hash, &node) < 0)
                printf("node_hash() failed with node_id : %ld \n", node.id);

            nodestate_tlv_size = tlv_nodestate(&nodestate_tlv, node.id, node.seqno,
                hash, node.data, node.data_len);
            if(nodestate_tlv_size <0)
                printf("malloc() failed in tlv_nodestate() : handle_tlv_nodestate_req \n");

            if(debug)
                printf("try to send node_state | node_id : %ld \n", node.id);

            send_packet_to(nodestate_tlv, nodestate_tlv_size, &dst_nbr, s);

            free(nodestate_tlv);
            return 0;
        }
    }

    printf("don't find corresponding node (id=%ld) in node_table", rec_id);

    return -1;
}

int handle_tlv_warning(u_int8_t *tlv){
    if(tlv[0] != TYPE_WARNING){
        printf("bad tlv_warning type! \n");
        return -1;
    }
    uint8_t len = tlv[1];
    if(len == 0)
        printf("Empty tlv_warning! \n");
    else
        printf("Warning: %.*s\n", len, tlv + 2);

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
    
    int rc =set_port(socket_peer, 4242);

    uint8_t response[PMTU];
   // handle_netstate_req test : 
    flood_net_hash(my_pair, socket_peer);
    
    struct sockaddr_storage sender_addr;
    socklen_t client_len = sizeof(sender_addr);
    rc = recvfrom(socket_peer,
            response, PMTU,
            0,
            (struct sockaddr*) &sender_addr, &client_len);

    printf("Received (%d bytes)\n",rc); 

    handle_tlv_netstate_req(my_pair, response +4, (struct sockaddr_in6 *)&sender_addr, socket_peer); 
    rc = recvfrom(socket_peer,
            response, PMTU,
            0,
            (struct sockaddr*) &sender_addr, &client_len);
    printf("Received (%d bytes)\n",rc); 
    printf(" ;; %d \n", response[4]);
    handle_tlv_nodesate_req(my_pair, response+4, (struct sockaddr_in6 *)&sender_addr, socket_peer);
    //test


    // rc = fcntl(socket_peer, F_GETFL);
    // if(rc < 0)
    //     perror("socket in core :");
    // rc = fcntl(socket_peer, F_SETFL, rc | O_NONBLOCK);
    // if(rc < 0)
    //     perror("socket in core :");


    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_peer, &master);
    SOCKET max_socket = socket_peer;



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
    //         printf("<- Response %d bytes: \n", rc);

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




//unit test tlv_nbr_req
    // struct sockaddr_storage sender_addr;
    // socklen_t client_len = sizeof(sender_addr);
    // rc = recvfrom(socket_peer,
    //         response, PMTU,
    //         0,
    //         (struct sockaddr*) &sender_addr, &client_len);

    // printf("Received (%d bytes)\n",rc); 
    // if(checkpkt_hd(response, rc) < 0){
    //     printf("Bad packet header\n");
    //     return 1;
    // }
    // handle_tlv_neighbour_req(my_pair, response +4,(struct sockaddr_in6*) &sender_addr,
    //     socket_peer);



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