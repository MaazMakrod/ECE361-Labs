#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#include <assert.h>

#define MAX_NAME 100
#define MAX_DATA 1024
#define MAX_MESSAGE 2048
#define MAX_SESSIONS 10

//For server and client
#define INVALID -1
#define LOGIN 0
#define LO_ACK 1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define NS_NAK 15
#define MESSAGE 10
#define QUERY 11
#define QU_ACK 12

//Client specific
#define QUIT 13
#define LOGOUT 14

#define LOGOUT_NO_CLOSE_THREAD 16

#define TRUE 1
#define FALSE 0

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct message* stringToMessage (char *messageString);
char* messageToString (struct message *p);
void printMessage (struct message *m);

struct clientNode {
  char *name;
  char *password;
  char *currentSession;
  int loggedIn;
  int socket_fd;
  pthread_t ptid;
};

struct commandReturn {
    unsigned int type;
    char *saveptr;
};

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
} while(0);

#endif