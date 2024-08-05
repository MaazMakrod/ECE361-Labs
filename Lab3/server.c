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

    srand(time(NULL));

    //Create the socket
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(socket_fd, "server creating socket");

    //create sock address, according to documentation do not need to set sin_zero to 0
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    check_error(bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "binding server socket");

    struct sockaddr_in client_info;
    socklen_t addrlen = sizeof(client_info);
    // int fd = -1;
    FILE *fp;

    int expectingPacket = 1;
    int expectingCount = 0;

    while(1){
        double r = (double) rand()/RAND_MAX;
        // printf("r is %f\n", r);

        char buffer[MAX_SIZE*2];
        int bytesReceived = recvfrom(socket_fd, buffer, MAX_SIZE*2, 0, (struct sockaddr *) &client_info, &addrlen);
        check_error(bytesReceived, "recv from client");

        if(r > 0.01){
            struct packet *p = stringToPacket(buffer);

            // printf("\nExpecting packet %d\n", expectingPacket);

            //We have already received and processed the packet
            if(p->frag_no == expectingPacket){
                // printf("Processing %d of %d: %s\n", p->frag_no, p->total_frag, p->filename);

                if(p->frag_no == 1){
                    char *fileName = malloc(MAX_SIZE);
                    strcat(fileName, "./received/");
                    strcat(fileName, p->filename);
                    fp = fopen(fileName, "w+");
                    free(fileName);
                }

                int bytesWritten = fwrite(p->filedata, 1, p->size, fp);
                check_error(bytesWritten, "Failed to write to file");                    

                expectingPacket++;
                expectingCount = 0;
            } else{
                printf("Already received packet %d, expecting %d\n", p->frag_no, expectingPacket);
                expectingCount++;
                
                if(expectingCount < 10)             
                    continue;
                
                expectingCount = 0;
                // exit(0);
            }

            // printf("sending ack for %d   ---> ", p->frag_no);
            char *ack = malloc(MAX_SIZE);
            char frag_num[MAX_SIZE];
            sprintf(frag_num, "%d", p->frag_no);

            strcat(ack, "ACK_");
            strcat(ack, frag_num);

            int sentBytes = sendto(socket_fd, ack, strlen(ack), 0, (struct sockaddr *) &client_info, addrlen);
            check_error(sentBytes, "sending ack");

            // printf("%s\n", ack);

            if(p->frag_no == p->total_frag){
                expectingPacket = 1;
                printf("Closing file\n");
                fclose(fp);
            }

            free(p);
            free(ack);
        } else
            printf("Dropping packet\n");
    }

    return 0;
}