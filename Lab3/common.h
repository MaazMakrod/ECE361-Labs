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
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

#define MAX_SIZE 1000
#define BILLION  1000000000.0

int errno;

struct packet {
    unsigned int total_frag; //Total number of fragments
    unsigned int frag_no; //Current fragment number
    unsigned int size; //Size of the file
    char* filename; //File name
    char filedata[1000]; //File data
};

struct packet* stringToPacket (char *packetString);
char* packetToString(struct packet *p);

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
} while(0);

#endif