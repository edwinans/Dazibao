#ifndef NETWROK_H
#define NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"


#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#define MOD (1<<16)
#define LARGER_MOD16(s1, s2) (!((s1 - s2) & (MOD/2)))
#define SUM_MOD16(s1, s2) ((s1 + s2) & (65535))

#define PMTU 1024
#define JCH_HOST "jch.irif.fr"
#define JCH_IPV6 "2001:660:3301:9200::51c2:1b9b"
#define JCH_IPV4 "81.194.27.155"
#define LOCAL_HOST "127.0.0.1"
#define UDP_DEF_PORT "1212"

#endif // !NETWROK_H