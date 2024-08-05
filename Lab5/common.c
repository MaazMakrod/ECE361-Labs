#include "common.h"
const char *delim = "^";

struct message* stringToMessage (char *messageString){
    // printf("%s\n", messageString);

    struct message *m = (struct message *) malloc(sizeof(struct message));
    char *saveptr;

    bzero(m->source, MAX_NAME);
    bzero(m->data, MAX_DATA);

    char *token = strtok_r(messageString, delim, &saveptr);
    int count = 1;

    while(token != NULL){
        // printf("%s %ld\n", token, strlen(token));
        if(count == 1){
            int type = atoi(token);
            m->type = type;
        } else if (count == 2){
            memcpy(&(m->source), token, strlen(token));
            // m->source = strdup(token);
        }else if (count == 3){
            m->size = atoi(token);
        } else if(count == 4) {
            memcpy(&(m->data), token, m->size);
            m->data[m->size] = '\0';
            // m->data = strdup(token);
            break;
        }

        if(count == 3)
            token = strtok_r(NULL, "", &saveptr);
        else
            token = strtok_r(NULL, delim, &saveptr);
        
        count++;
    }

    // printf("STRUCT CONTENTS: %d %d %s %s\n", m->size, m->type, m->data, m->source);

    return m;
}

//Messages are encoded Type:Source:Size:Data
char* messageToString (struct message *m){
    char *messageString = (char *) malloc(MAX_MESSAGE);
    bzero(messageString, MAX_MESSAGE);
    // printf("%s\n", messageString);
    
    char size[10];
    sprintf(size, "%d", m->size);

    char type[10];
    sprintf(type, "%d", m->type);

    // printf("%s %s %s %s\n\n", type, m->source, size, m->data);

    strcat(messageString, type);
    // printf("%s\n\n", messageString);
    strcat(messageString, delim);
    strcat(messageString, (char *) m->source);
    strcat(messageString, delim);
    strcat(messageString, size);
    strcat(messageString, delim);
    strcat(messageString, (char *) m->data);

    // printf("%s - %ld - %ld\n", messageString, strlen(messageString), sizeof(messageString));

    //Fill in the rest of the buffer
    for(int i = strlen(messageString); i < MAX_MESSAGE; i++){
        messageString[i] = '-';
    }

    // printf("%s - %ld - %ld\n", messageString, strlen(messageString), sizeof(messageString));

    return messageString;
}

void printMessage (struct message *m){
    for(int i = 0; i < m->size; i++){
        printf("%c", m->data[i]);
    }
}