#ifndef __BITS_SOCKET_H__
#define __BITS_SOCKET_H__

#include <bits/types.h>

#define AF_UNIX        1
#define AF_NETLINK    16

#define PF_UNSPEC      0
#define PF_UNIX        1
#define PF_NETLINK    16

#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3
#define SOCK_CLOEXEC   02000000
#define SOCK_NONBLOCK  04000

#define SOL_SOCKET      1
#define SOL_NETLINK	270
#define SO_PEERCRED     17

#define SHUT_WR 1

typedef unsigned short sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[14];
};

struct ucred {
	pid_t pid;
	uid_t uid;
	gid_t gid;
};

#endif