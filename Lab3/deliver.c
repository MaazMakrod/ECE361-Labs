#include "common.h"

/*
Avg RTT Times for a 100 M file

0.000213
0.000203
0.000206
0.000210
0.000211

Avg of the averages is 0.0002086 sec or 208.6 microseconds

We will use this as our initial Estimated RTT time and calculate a new
RTT time using the formula provided in class

EstimatedRTT = (1-a)*EstimatedRTT + a*SampleRTT, a is 0.125

In 5 trials, the final value of EstimatedRTT was as follows:

0.000180
0.000219
0.000175
0.000170
0.000189

Avg EstimatedRTT is 0.0001866

Next we calculated DevRTT, starting at an initial val of 0.
The formula is the same as what was introduced in class

DevRTT = (1-b)*DevRTT + b*|SampleRTT-EstimatedRTT|, b=0.25

5 trials led the value of DevRTT to be

0.000021
0.000040
0.000026
0.000030
0.000026

Avg. DevRTT Time is 0.0000286

Finally using TimeoutInterval = EstimatedRtt + 4*DevRTT we get that the
timeout interval is 0.000301 sec or 301 microseconds

For a further safety margin, we will multiply this by 10 to have a timeout of 3010 microseconds

How do we deal with the low chance that the packet is being processed and an ack is sent but 
the socket has timed out. So it resends the packet, and then the ack comes twice for the same packet
*/

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
    // struct timespec start_time, end_time;
    // double totalTime = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    check_error(getaddrinfo(argv[1], argv[2], &hints, &servinfo), "getting address info");
    
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(socket_fd, "creating socket");

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 3010; //0.003010 sec from experimentation
    check_error(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)), "setting timeout");

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
        int bytesReceived = 0;

        struct packet *p = (struct packet *) malloc(sizeof(struct packet));

        while(num_read = fread(buffer, 1, MAX_SIZE, fp), num_read > 0){
            p->filename = fileName;
            p->size = num_read;

            memcpy(p->filedata, buffer, p->size);

            p->total_frag = totalPackets;
            p->frag_no = packetNum;

            packetNum++;

            char *packetString = packetToString(p);
            // clock_gettime(CLOCK_REALTIME, &start_time);

            do{
                if(bytesReceived == -1)
                    printf("Timed out sending packet %d\n", p->frag_no);
                
                int sentBytes = sendto(socket_fd, packetString, 4096, 0, servinfo->ai_addr, servinfo->ai_addrlen);
                check_error(sentBytes, "sending packet");

                socklen_t addrlen = servinfo->ai_addrlen;

                bytesReceived = recvfrom(socket_fd, bufferRes, MAX_SIZE, 0, servinfo->ai_addr, &addrlen);
                // printf("Received: %d\n", bytesReceived);

            } while(bytesReceived == -1);

            bufferRes[bytesReceived] = '\0';

            // char *ack = malloc(MAX_SIZE);
            // char frag_num[MAX_SIZE];
            // sprintf(frag_num, "%d", p->frag_no);

            // strcat(ack, "ACK_");
            // strcat(ack, frag_num);

            // printf("%s --> %s ---- ", bufferRes, ack);

            // if(strcmp(bufferRes, ack) != 0){
            //     printf("DONT MATCH\n");
            //     exit(0);
            // }           

            // printf("\n");

            // free(packetString);
        }

        free(p);
    } else
        printf("File not found\n");

    free(fileName);
    return 0;
}