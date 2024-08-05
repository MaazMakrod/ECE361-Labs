#include "common.h"

static void usage(char *program) {
	fprintf(stderr, "Usage: %s <port_number> \n", program);
	exit(1);
}

int main(int argc, char *argv[]){
    if(argc != 2)
        usage(argv[0]);

    int port = atoi(argv[1]);

    if(port < 1024) {
		fprintf(stderr, "port = %d, should be >= 1024\n", port);
		usage(argv[0]);
	}

    //Create the socket
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(socket_fd, "server creating socket");

    //create sock address, according to documentation do not need to set sin_zero to 0
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    check_error(bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "binding server socket");

    char *buf = (char *) malloc(MAX_SIZE);
    struct sockaddr_in client_info;
    socklen_t addrlen = sizeof(client_info);

    while(1){
        int bytesReceived = recvfrom(socket_fd, buf, MAX_SIZE, 0, (struct sockaddr *) &client_info, &addrlen);

        check_error(bytesReceived, "recv from client");

        if(bytesReceived == 3 && buf[0] == 'f' && buf[1] == 't' && buf[2] == 'p'){
            int sentBytes = sendto(socket_fd, "yes", strlen("yes"), 0, (struct sockaddr *) &client_info, addrlen);
            check_error(sentBytes, "sending yes res");
        } else {
            int sentBytes = sendto(socket_fd, "no", strlen("no"), 0, (struct sockaddr *) &client_info, addrlen);
            check_error(sentBytes, "sending no res");
        }
    }

    return 0;
}