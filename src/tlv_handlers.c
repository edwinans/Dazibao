#include "../include/tlv_handlers.h"

bool debug;


void print_node(node_t node){
    printf("node_%ld : (%ld, %d, %.*s) \n", node.id, node.id, node.seqno, node.data_len, node.data);
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
        printf("[chckpkt] ; body-len :%d , size : %d\n", body_len, size);
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
        printf("-> sending packet to nbr ->");
        print_addr(nbr->addr);
    }
    int rc = sendto(s, packet, sizeof(packet), 0, (struct sockaddr*) nbr->addr, sizeof(struct sockaddr_in6));
    if(rc < 0 || rc>PMTU){
        fprintf(stderr, "error sending packet , nb_bytes: %d ; %s\n", rc, strerror( errno ));
        return -1;
    }
        

    return rc;
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
    
    node_id rec_id;
    memcpy(&rec_id, offset, sizeof(node_id));
    //rec_id = be64toh(hid);
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
    uint16_t nreq_tlv_size = tlv_nodestate_req(&nodestate_req_tlv, rec_id);
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
    if(tlv[0] != TYPE_NODESTATE_REQ){
        printf("bad tlv type : NODE_STATE_REQ \n");
        return -1;
    }
    uint8_t tlv_len = tlv[1];
    if(tlv_len != sizeof(node_id)){
        printf("incorrect tlv len: %u in handle_tlv_nodestate_req() \n", tlv_len);
        return -1;
    }

    node_id rec_id;
    memcpy(&rec_id, tlv + 2, sizeof(node_id));
    uint8_t hash[HASH_SIZE*2];

    uint8_t *nodestate_tlv;
    uint16_t nodestate_tlv_size;
    neighbour_t dst_nbr = {0, 0, src_addr};

    for(int i=0; i< pair->nb_nodes; i++){
        node_t node = pair->nodes[i];

        if(rec_id == node.id){
            if(debug)
                print_node(node);
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

    printf("don't find corresponding node (id=%ld) in node_table \n", rec_id);

    return -1;
}

int handle_tlv_nodestate(pair_t *pair, uint8_t *tlv, struct sockaddr_in6 *src_addr, SOCKET s){
    if(tlv[0] != TYPE_NODESTATE){
        printf("bad tlv type : NODE_STATE \n");
        return -1;
    }
    uint8_t tlv_len = tlv[1];
    if(tlv_len < sizeof(node_id) + sizeof(seq_n) + HASH_SIZE){
        printf("tlv_len = %u < min tlv_len of node_state;\
            in function handle_tlv_nodestate() \n", tlv_len);
        return -1;
    }
    uint8_t datalen = tlv_len - sizeof(node_id) - sizeof(seq_n) - HASH_SIZE;
    if(datalen > MAX_DATA_LEN){
        printf("datalen = %u is larger than MAX_DATA_LEN = %d ;\
            in function handle_tlv_nodestate() \n", datalen, MAX_DATA_LEN);
        return -1;
    }
    uint8_t *offset = tlv + HEADER_OFFSET;
    
    node_id rec_id;
    memcpy(&rec_id, offset, sizeof(node_id));
    offset += sizeof(node_id);

    seq_n nseqno, rec_seqno; 
    memcpy(&nseqno, offset, sizeof(seq_n));
    rec_seqno = ntohs(nseqno);
    offset += sizeof(seq_n);

    uint8_t rec_hash[HASH_SIZE*2], my_hash[HASH_SIZE*2];
    memcpy(rec_hash, offset, HASH_SIZE);
    offset += HASH_SIZE;

    uint8_t rec_data[MAX_DATA_LEN] = {0};
    memcpy(rec_data, offset, datalen);

    int target_index=0;

    for(int i=0; i< pair->nb_nodes; i++){
        target_index = i;
        node_t node = pair->nodes[i];

        if(node.id > rec_id) //nodes[] should be dynamically sorted
            break;

        if(node.id == rec_id){
            if(node_hash(my_hash, &node) < 0)
                printf("node_hash() failed in handle_tlv_nodestate() \n");

            if(!hash_equals(my_hash, rec_hash)){
                if(debug)
                    printf("recvd node_hash not equals for node_id: %ld \n", node.id);
                
                if(node.id == pair->id){
                    if(LARGER_MOD16(rec_seqno, node.seqno)){
                        pair->nodes[i].seqno = SUM_MOD16(rec_seqno, 1);

                        if(debug){
                            printf("seqno_replace_to(%d, %d) of node_%ld \n"\
                                , node.seqno, pair->nodes[i].seqno, node.id);
                        }
                    }
                    //node correspond to own pair->node
                }
                else{
                    if(!(LARGER_MOD16(node.seqno, rec_seqno))){
                        if(debug){
                            printf("update existing node_%ld ... \n", node.id);
                        }

                        pair->nodes[i].seqno = rec_seqno;
                        pair->nodes[i].data_len = datalen;
                        memcpy(pair->nodes[i].data, rec_data, MAX_DATA_LEN);
                    }
                    //update node if necessary   
                }
                //corresponding node found
            }

            return 0;
        }
    }

    //corresponding node not found 
    if(pair->nb_nodes >= pair->nodes_len){ //nb of nodes reached the capacity
        node_t *tmp_nodes = realloc(pair->nodes, 2 * sizeof(node_t) * pair->nodes_len);
        if(tmp_nodes == NULL){
            printf("realloc() failed in handle_tlv_nodetate() ; \n");
            return -1;
        }

        pair->nodes = tmp_nodes;
        pair->nodes_len *=2;
    }

    memmove(pair->nodes + target_index + 1, 
        pair->nodes + target_index,
        sizeof(node_t) * (pair->nb_nodes - target_index));
    
    pair->nb_nodes++;
    pair->nodes[target_index].id = rec_id;
    pair->nodes[target_index].seqno = rec_seqno;
    pair->nodes[target_index].data_len =datalen;
    memcpy(pair->nodes + target_index, rec_data, MAX_DATA_LEN);

    if(debug){
        printf("node added successfully ::  \n");
        print_node(pair->nodes[target_index]);
    }

    return 0;
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
