#ifndef NETWROK_H
#define NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#define JCH_HOST "jch.irif.fr"
#define JCH_IPV6 "001:660:3301:9200::51c2:1b9b"
#define JCH_IPV4 "81.194.27.155"
#define UDP_DEF_PORT "1212"

#endif // !NETWROK_H