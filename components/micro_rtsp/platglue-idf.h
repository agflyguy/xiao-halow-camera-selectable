#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "esp_random.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

typedef std::string String;

typedef int SOCKET;
typedef int UDPSOCKET;
typedef uint32_t IPADDRESS;
typedef uint16_t IPPORT;

#define NULLSOCKET 0

#define getRandom() ((int)esp_random())

inline void socketpeeraddr(SOCKET s, IPADDRESS *addr, IPPORT *port)
{
    struct sockaddr_in r;
    socklen_t len = sizeof(r);
    if (getpeername(s, (struct sockaddr *)&r, &len) < 0) {
        *addr = 0;
        *port = 0;
        return;
    }
    *port = ntohs(r.sin_port);
    *addr = r.sin_addr.s_addr;
}

inline void udpsocketclose(UDPSOCKET s)
{
    if (s != NULLSOCKET) {
        close(s);
    }
}

inline UDPSOCKET udpsocketcreate(unsigned short portNum)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        return NULLSOCKET;
    }

    addr.sin_port = htons(portNum);
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(s);
        return NULLSOCKET;
    }

    return s;
}

inline ssize_t socketsend(SOCKET sockfd, const void *buf, size_t len)
{
    return send(sockfd, buf, len, 0);
}

inline ssize_t udpsocketsend(UDPSOCKET sockfd, const void *buf, size_t len, IPADDRESS destaddr,
                             uint16_t destport)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = destaddr;
    addr.sin_port = htons(destport);
    return sendto(sockfd, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
}

inline int socketread(SOCKET sock, char *buf, size_t buflen, int timeoutmsec)
{
    struct timeval tv;
    tv.tv_sec = timeoutmsec / 1000;
    tv.tv_usec = (timeoutmsec % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int res = recv(sock, buf, buflen, 0);
    if (res > 0) {
        return res;
    }
    if (res == 0) {
        return 0;
    }
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
        return -1;
    }
    return 0;
}
