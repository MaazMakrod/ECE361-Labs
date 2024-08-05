#include "common.h"

static void usage(char *program) {
	fprintf(stderr, "Usage: %s <server_address> <port_number>\n", program);
	exit(1);
}

int main(int argc, char *argv[]){
    if(argc != 3)
        usage(argv[0]);

    int port = atoi(argv[2]);
    // char *address = argv[1];

    if(port < 1024) {
		fprintf(stderr, "port = %d, should be >= 1024\n", port);
		usage(argv[0]);
	}

    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    check_error(getaddrinfo(argv[1], argv[2], &hints, &servinfo), "getting address info");
    
    // int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(socket_fd, "creating socket");
    // printf("socket fd %d\n", socket_fd);

    // struct sockaddr_in *servaddr = (struct sockaddr_in *) &(servinfo->ai_addr);
    // void *ip_addr = &(servaddr->sin_addr);
    // char *ipver = "IPv4";
    
    // char ipstr[INET6_ADDRSTRLEN];
    // // convert the IP to a string and print it:
    // inet_ntop(servinfo->ai_family, ip_addr, ipstr, sizeof ipstr);
    // printf(" %s: %s\n", ipver, ipstr);

    //((struct sockaddr_in *)servinfo->ai_addr)
    char input[MAX_SIZE], fileName[MAX_SIZE];
    int fileNameStart = 0;
    scanf("%[^\n]%*c", input);

    for(int i = 0; i < strlen(input); i++){
        if(input[i] != ' ')
            break;
        
        fileNameStart++;
    }

    if(strncmp(&input[fileNameStart], "ftp ", 4) != 0){
        printf("Invalid input\n");
        return 1;
    }

    fileNameStart+=4;

    for(int i = fileNameStart; i < strlen(input); i++){
        if(input[i] != ' ')
            break;
        
        fileNameStart++;
    }

    strcpy(fileName, &(input[fileNameStart]));

    if(access(fileName, F_OK) == 0){
        int sentBytes = sendto(socket_fd, "ftp", strlen("ftp"), 0, servinfo->ai_addr, servinfo->ai_addrlen);
        check_error(sentBytes, "sending ftp");

        char *buf = (char *) malloc(MAX_SIZE);
        socklen_t addrlen = servinfo->ai_addrlen;

        int bytesReceived = recvfrom(socket_fd, buf, MAX_SIZE, 0, servinfo->ai_addr, &addrlen);
        check_error(bytesReceived, "recv from server");

        // printf("received %d bytes\n", bytesReceived);
        
        for(int i = 0; i < bytesReceived; i++)
            printf("%c", buf[i]);

        printf("\n");
    } else
        printf("File not found\n");

    return 0;
}