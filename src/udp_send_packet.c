#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)


int main() {


    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if (getaddrinfo("jch.irif.fr", "1212", &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
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
    // local.sin_port = htons(4568);


    // printf("Binding socket to corresponding port...\n");
    // if ( bind(socket_peer,(struct sockaddr*)&local, sizeof(local))) {
    //     fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    //     return 1;
    // }
    // //


    //const char *message = "Hello World";
    uint8_t packet[4];
    packet[0] = 95;
    packet[1] = 1;
    uint16_t zero = 0;
    memcpy(packet+2, &zero, 2);

    printf("Sending packet: ");
    for (int i = 0; i < 4; i++){
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
    printf("%02X", reply[0]);

    freeaddrinfo(peer_address);
    CLOSESOCKET(socket_peer);


    printf("Finished.\n");
    return 0;
}