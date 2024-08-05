#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIZE 100

int errno;

#define check_error(ret, message) do { \
    if (ret != -1) { \
        ; \
    } else {\
        \
        char error [50]; \
        sprintf(error, "Failed to perform %s - %d", message, errno); \
        \
        perror(error); \
        exit(errno); \
    } \
} while(0)

#endif