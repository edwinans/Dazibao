#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/network.h"

int main() {


    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    struct addrinfo *peer_address;
    if (getaddrinfo(LOCAL_HOST, "4242", &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    for(struct addrinfo *p = peer_address; p != NULL; p = p->ai_next) {
        printf("addr: %d, len: %d\n", p->ai_addr->sa_family, p->ai_addrlen);
    }


    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST  | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);


    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }



    // //set port
    // struct sockaddr_in local = {};
    // local.sin_family = AF_INET;
    // local.sin_port = htons(1212);


    // printf("Binding socket to corresponding port...\n");
    // if ( bind(socket_peer,(struct sockaddr*)&local, sizeof(local))) {
    //     fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    //     return 1;
    // }
    // //


    //const char *message = "Hello World";
    uint8_t packet[6];
    packet[0] = 95;
    packet[1] = 1;
    uint16_t hlen = htons(2);
    memcpy(packet+2, &hlen, sizeof(hlen));
    packet[4]= 2;
    packet[5]=0;

    printf("Sending packet: ");
    for (int i = 0; i < sizeof(packet); i++){
        if (i > 0) printf(":");
            printf("%02X", packet[i]);
    }
    printf("\n");

    int bytes_sent = sendto(socket_peer,
            packet, sizeof(packet),
            0,
            peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Sent %d bytes.\n", bytes_sent);

    uint8_t reply[1024];
    int rc = recvfrom(socket_peer, reply, 4096, 0, NULL, NULL);

    printf("Response %d bytes: \n", rc);
    for (int i = 0; i < rc; i++){
        if (i > 0) printf(":");
            printf("%02X", reply[i]);
    }
    printf("\n");

    freeaddrinfo(peer_address);
    CLOSESOCKET(socket_peer);


    printf("Finished.\n");
    return 0;
}