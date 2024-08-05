#include "common.h"

//String to packet function
//Replace semi colon with a '\0'
//other wise can get the length of each portion and go off that
struct packet* stringToPacket(char *packetString){
    struct packet *p = (struct packet *) malloc(sizeof(struct packet));
    
    int colonCount = 0;
    int i = 0, s_index = 0;

    // for(int j = 0; j < 1050; j++){
    //     printf("%c", packetString[j]);
    // }

    // printf("\n\n==============\n\n");

    while(colonCount < 4){
        if(packetString[i] == ':'){
            packetString[i] = '\0';
            colonCount++;
            i++;

            char *extractedString = strdup(&(packetString[s_index]));

            packetString[i-1] = ':';
            
            if(colonCount == 1)
                p->total_frag = atoi(extractedString);
            else if(colonCount == 2)
                p->frag_no = atoi(extractedString);
            else if(colonCount == 3)
                p->size = atoi(extractedString);
            else
                p->filename = extractedString;

            // printf("%s\n", n);
            s_index = i;
        }
        i++;
    }

    // char *n = malloc(p->size);
    // memcpy(n, &(packetString[s_index]), p->size);
    // printf("%s\n\n\n", n);

    // for(int i = 0; i < s_index; i++){
    //     printf("%c", packetString[i]);
    // }
    // printf("\n");
    
    memcpy(p->filedata, &(packetString[s_index]), p->size);

    return p;
}

//packet to string function
char* packetToString(struct packet *p){
    char *data = (char *) malloc(4096);

    char total_frag[MAX_SIZE];
    char frag_num[MAX_SIZE];
    char size[MAX_SIZE];

    sprintf(total_frag, "%d", p->total_frag);
    sprintf(frag_num, "%d", p->frag_no);
    sprintf(size, "%d", p->size);

    strcat(data, total_frag);
    strcat(data, ":");
    strcat(data, frag_num);
    strcat(data, ":");
    strcat(data, size);
    strcat(data, ":");
    strcat(data, p->filename);
    strcat(data, ":");

    // printf("%s\n", data);

    int data_start = 4 + strlen(total_frag) + strlen(size) + strlen(frag_num) + strlen(p->filename);
    memcpy(&(data[data_start]), p->filedata, p->size);

    // printf("%s\n", data);

    return data;
}