#include "common.h"

static void usage(char *program) {
	fprintf(stderr, "Usage: %s <server_address> <port_number>\n", program);
	exit(1);
}

char* extractFileName(){
    char input[MAX_SIZE];
    char *fileName = malloc(MAX_SIZE);
    int fileNameStart = 0;

    scanf("%[^\n]%*c", input);

    for(int i = 0; i < strlen(input); i++){
        if(input[i] != ' ')
            break;
        
        fileNameStart++;
    }

    if(strncmp(&input[fileNameStart], "ftp ", 4) != 0){
        printf("Invalid input\n");
        exit(1);
    }

    fileNameStart+=4;

    for(int i = fileNameStart; i < strlen(input); i++){
        if(input[i] != ' ')
            break;
        
        fileNameStart++;
    }

    strcpy(fileName, &(input[fileNameStart]));

    return fileName;
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
    struct timespec start_time, end_time;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    check_error(getaddrinfo(argv[1], argv[2], &hints, &servinfo), "getting address info");
    
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(socket_fd, "creating socket");

    char* fileName = extractFileName();

    if(access(fileName, F_OK) == 0){

        FILE *fp = fopen(fileName, "r");
        
        struct stat st;
        stat(fileName, &st);
        int totalSize = st.st_size;

        int totalPackets = ceil((double)totalSize/MAX_SIZE);
        int packetNum = 1;        
        int num_read = 0;
        char buffer[MAX_SIZE];
        char bufferRes[MAX_SIZE];
        // double totalTime = 0;
        double EstimatedRTT = 0.0001866;
        double DevRTT = 0;

        struct packet *p = (struct packet *) malloc(sizeof(struct packet));

        while(num_read = fread(buffer, 1, MAX_SIZE, fp), num_read > 0){
            p->filename = fileName;
            p->size = num_read;

            memcpy(p->filedata, buffer, p->size);

            p->total_frag = totalPackets;
            p->frag_no = packetNum;

            packetNum++;

            char *packetString = packetToString(p);

            clock_gettime(CLOCK_REALTIME, &start_time);

            int sentBytes = sendto(socket_fd, packetString, 4096, 0, servinfo->ai_addr, servinfo->ai_addrlen);
            check_error(sentBytes, "sending packet");

            socklen_t addrlen = servinfo->ai_addrlen;

            int bytesReceived = recvfrom(socket_fd, bufferRes, MAX_SIZE, 0, servinfo->ai_addr, &addrlen);
            check_error(bytesReceived, "recv from server");

            clock_gettime(CLOCK_REALTIME, &end_time);
            double time_spent = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / BILLION;
            printf("The RTT time is %f\n", time_spent);
            
            // EstimatedRTT = EstimatedRTT*(1.0-0.125) + 0.125 * time_spent;
            // printf("Current EstimatedRTT: %f\n", EstimatedRTT);

            // totalTime += time_spent;

            DevRTT = DevRTT*(1.0-0.25) + 0.25*fabs(EstimatedRTT-time_spent);
            printf("Current DevRTT: %f\n", DevRTT);

            for(int i = 0; i < bytesReceived; i++)
                printf("%c", bufferRes[i]);

            printf("\n");
        }

        // printf("The total trip time is %f and the avg RTT time is %f\n", totalTime, totalTime/totalPackets);
        // printf("Final EstimatedRTT: %f\n", EstimatedRTT);
        printf("Final DevRTT: %f\n", DevRTT);

        free(p);
    } else
        printf("File not found\n");

    free(fileName);
    return 0;
}