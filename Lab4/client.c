#include "common.h"

void startupClient();
struct commandReturn* extractCommand(char* input);
int performCommand(struct commandReturn *command);
void usage(char *program);
int login(struct commandReturn *command);
int logout(struct commandReturn *command);
int quit(struct commandReturn *command);
int query(struct commandReturn *command);
int createSession(struct commandReturn *command);
int joinSession(struct commandReturn *command);
int leaveSession(struct commandReturn *command);
int message(struct commandReturn *command);
void *serverHandler();
void clientInputHandler();
int checkLoggedIn();

struct clientNode client;

pthread_mutex_t client_lock;

void usage(char *program) {
	fprintf(stderr, "Usage: %s\n", program);
	exit(1);
}

int checkLoggedIn(){
  if(!client.loggedIn){
    printf("Error: client must be logged in\n");
    return FALSE;
  }

  return TRUE;
}

struct commandReturn* extractCommand(char* input){
  int commandStart = 0;

  struct commandReturn* retC = (struct commandReturn*) malloc(sizeof(struct commandReturn));
  retC->type = INVALID;

  for(int i = 0; i < strlen(input); i++){
        if(input[i] != ' ')
            break;
        
        commandStart++;
  }

  if(input[commandStart] != '/'){
    retC->type = MESSAGE;
    retC->saveptr = input;
    return retC;
  }

  // strcpy(input, &(input[commandStart]));
  char *command = strtok_r(input, " ", &(retC->saveptr));

  if(strcmp(command, "/login") == 0)
    retC->type = LOGIN;

  if(strcmp(command, "/quit") == 0)
    retC->type = QUIT;

  if(strcmp(command, "/logout") == 0)
    retC->type = LOGOUT;

  if(strcmp(command, "/joinsession") == 0)
    retC->type = JOIN;
  
  if(strcmp(command, "/createsession") == 0)
    retC->type = NEW_SESS;

  if(strcmp(command, "/leavesession") == 0)
    retC->type = LEAVE_SESS;
  
  if(strcmp(command, "/list") == 0)
    retC->type = QUERY; 

  return retC; 
}

int createSession(struct commandReturn *command){
  char *session_id = strtok_r(NULL, " ", &(command->saveptr));
  free(command);

  if(!checkLoggedIn())
    return INVALID;

  if(session_id == NULL){
    printf("Usage: /createsession <session id>\n");
    return INVALID;
  }

  // printf("%s\n", session_id);

  struct message *m = (struct message *) malloc(sizeof(struct message));
  bzero(m->source, MAX_NAME);
  bzero(m->data, MAX_DATA);
  m->type = NEW_SESS;
  m->size = strlen(session_id);
  memcpy(&(m->source), client.name, strlen(client.name));
  memcpy(&(m->data), session_id, m->size);

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  char *message = messageToString(m);
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);
  free(m);

  check_error(send(client.socket_fd, buffer, sizeof(buffer), 0), "sending to server");

  return NEW_SESS;
}

int joinSession(struct commandReturn *command){
  char *session_id = strtok_r(NULL, " ", &(command->saveptr));
  free(command);

  if(!checkLoggedIn())
    return INVALID;

  if(session_id == NULL){
    printf("Usage: /joinsession <session id>\n");
    return INVALID;
  }

  struct message *m = (struct message *) malloc(sizeof(struct message));
  bzero(m->source, MAX_NAME);
  bzero(m->data, MAX_DATA);
  m->type = JOIN;
  m->size = strlen(session_id);
  memcpy(&(m->source), client.name, strlen(client.name));
  memcpy(&(m->data), session_id, m->size);

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  char *message = messageToString(m);
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);
  free(m);

  check_error(send(client.socket_fd, buffer, sizeof(buffer), 0), "sending to server");

  return JOIN;
}

int leaveSession(struct commandReturn *command){
  free(command);

  if(!checkLoggedIn())
    return INVALID;

  if(client.currentSession == NULL){
    printf("Error: Client is not currently in a session\n");
    return INVALID;
  }

  pthread_mutex_lock(&client_lock);
  free(client.currentSession);
  client.currentSession = NULL;
  pthread_mutex_unlock(&client_lock);

  struct message *m = (struct message *) malloc(sizeof(struct message));
  bzero(m->source, MAX_NAME);
  bzero(m->data, MAX_DATA);
  m->type = LEAVE_SESS;
  m->size = 0;
  memcpy(&(m->source), client.name, strlen(client.name));

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  char *message = messageToString(m);
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);
  free(m);

  check_error(send(client.socket_fd, buffer, sizeof(buffer), 0), "sending to server");

  return LEAVE_SESS;
}

int message(struct commandReturn *command){
  // printf("%s\n", command->saveptr);
  if(!checkLoggedIn())
    return INVALID;

  if(client.currentSession == NULL){
    printf("Error: Must be in a session to send a message\n");
    free(command);
    return INVALID;
  }

  struct message *m = (struct message *) malloc(sizeof(struct message));
  bzero(m->source, MAX_NAME);
  bzero(m->data, MAX_DATA);
  m->type = MESSAGE;
  m->size = strlen(command->saveptr);
  memcpy(&(m->source), client.name, strlen(client.name));
  memcpy(&(m->data), command->saveptr, strlen(command->saveptr));

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  char *message = messageToString(m);
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);
  free(m);

  check_error(send(client.socket_fd, buffer, sizeof(buffer), 0), "sending to server");

  free(command);
  return MESSAGE;
}

int performCommand(struct commandReturn *command){
  // assert(client.loggedIn);

  switch(command->type){
    case MESSAGE:
      return message(command);//printf("The MESSAGE command was entered\n");
      break;
    case LOGIN:
      return login(command);
      break;
    case QUIT:
      return quit(command);
      break;
    case LOGOUT:
      return logout(command);// printf("The LOGOUT command was entered\n");
      break;
    case JOIN:
      return joinSession(command);//printf("The JOIN command was entered\n");
      break;
    case NEW_SESS:
      return createSession(command);//printf("The NEW_SESS command was entered\n");
      break;
    case LEAVE_SESS:
      return leaveSession(command);//printf("The LEAVE_SESS command was entered\n");
      break;
    case QUERY:
      return query(command);
      break;
  }  

  free(command);
  return INVALID;
}

int login(struct commandReturn *command){
  if(client.loggedIn){
    printf("Error: Client is already logged in as %s\n", client.name);
    free(command);
    return -1;
  }

  char *clientId = strtok_r(NULL, " ", &(command->saveptr));
  char *password = strtok_r(NULL, " ", &(command->saveptr));
  char *serverName = strtok_r(NULL, " ", &(command->saveptr));
  char *port = strtok_r(NULL, " ", &(command->saveptr));
  free(command);

  if(port == NULL){
    printf("Usage: /login <client id> <password> <server address> <server port>\n");
    return -1;
  }

  int portNum = atoi(port);

  if(portNum < 1024) {
    fprintf(stderr, "port = %d, should be >= 1024\n", portNum);
    return -1;
  }

  // printf("%s %s %s %s\n", clientId, password, serverName, port);

  struct addrinfo hints, *server_info, *p;
  int socket_fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  check_error(getaddrinfo(serverName, port, &hints, &server_info), "getting address info");

  for(p = server_info; p != NULL; p = p->ai_next){
    if(( socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol) ) == -1)
      continue;

    if(connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
      close(socket_fd);
      continue;
    }

    break;
  }

  freeaddrinfo(server_info);

  if(p == NULL){
    printf("Could not create socket or bind\n");
    exit(1);
  }

  struct message *m = (struct message *) malloc(sizeof(struct message));
  bzero(m->data, MAX_DATA);
  bzero(m->source, MAX_NAME);

  m->type = LOGIN;
  m->size = strlen(password);
  memcpy(&(m->source), clientId, strlen(clientId));
  memcpy(&(m->data), password, m->size);

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  char *message = messageToString(m);
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);

  // printf("%s\n", buffer);
  check_error(send(socket_fd, buffer, sizeof(buffer), 0), "sending to server");

  bzero(buffer, MAX_MESSAGE);
  int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
  check_error(bytes_received, "receiving from server");

  // for(int i = 0; i < bytes_received; i++)
  //   printf("%c", buffer[i]);
  // printf("\n");

  free(m);
  m = stringToMessage(buffer);

  if(m->type == LO_NAK){
    printf("%s\n", m->data);
    close(socket_fd);
    free(m);

    return -1;
  }

  free(m);

  pthread_mutex_lock(&client_lock);
  client.name = strdup(clientId);
  client.password = strdup(password);
  client.loggedIn = TRUE;
  client.socket_fd = socket_fd;
  pthread_mutex_unlock(&client_lock);

  pthread_create(&client.ptid, NULL, &serverHandler, NULL);

  return socket_fd;
}

void startupClient(){
  pthread_mutex_lock(&client_lock);
  client.loggedIn = FALSE;
  client.name = NULL;
  client.currentSession = NULL;
  client.password = NULL;
  client.socket_fd = -1;
  pthread_mutex_unlock(&client_lock);

  char* input = (char *) malloc(MAX_DATA);

  while(!client.loggedIn){
    // printf("> ");
    scanf("%[^\n]%*c", input);
    struct commandReturn *command = extractCommand(input);

    if(command->type != LOGIN && command->type != QUIT){
      printf("Please login using /login or quit using /quit\n");
      continue;
    }

    if(command->type == LOGIN){
      if(login(command) < 0)
        continue;

      break;
    }

    quit(command);
  }
}

int logout(struct commandReturn *command){
  if(!checkLoggedIn())
    return INVALID;

  pthread_mutex_lock(&client_lock);

  if(command->type != LOGOUT_NO_CLOSE_THREAD) {
    pthread_cancel(client.ptid);

    struct message *m = (struct message *) malloc(sizeof(struct message));
    bzero(m->source, MAX_NAME);
    bzero(m->data, MAX_DATA);

    m->type = EXIT;
    m->size = 0;
    memcpy(&(m->source), client.name, strlen(client.name));

    char *message = messageToString(m);
    check_error(send(client.socket_fd, message, sizeof(message), 0), "sending to exit to server");

    free(m);
    free(message);
  }
  
  free(command);
  free(client.name);
  free(client.password);

  client.name = NULL;
  client.password = NULL;
  client.loggedIn = FALSE;

  close(client.socket_fd);

  if(client.currentSession != NULL){
    free(client.currentSession);
    client.currentSession = NULL;
  }

  pthread_mutex_unlock(&client_lock); 
  return LOGOUT;
}

int quit(struct commandReturn *command){
  
  if(client.loggedIn){
    logout(command);
  } else
    free(command);
  
  pthread_mutex_destroy(&client_lock);

  exit(0);
}

int query(struct commandReturn *command) {
  if(!checkLoggedIn())
    return INVALID;
    
  free(command);

  struct message *m = (struct message *) malloc(sizeof(struct message));
  bzero(m->source, MAX_NAME);
  bzero(m->data, MAX_DATA);

  m->type = QUERY;
  m->size = 0;
  memcpy(&(m->source), client.name, strlen(client.name));

  char *message = messageToString(m);
  check_error(send(client.socket_fd, message, sizeof(message), 0), "sending to exit to server");  
  free(m);
  free(message);

  return QUERY;
}

//Prints messages received from server
void *serverHandler(){
  char buffer[MAX_MESSAGE];
  int bytes_received;

  while(1){
    bzero(buffer, MAX_MESSAGE);

    bytes_received = recv(client.socket_fd, buffer, sizeof(buffer), 0);
    check_error(bytes_received, "receive from server");

    if(bytes_received == 0){
      printf("Server has closed, logging out client\n");
      struct commandReturn *c = malloc(sizeof(struct commandReturn));
      c->type = LOGOUT_NO_CLOSE_THREAD;
      logout(c);
      pthread_cancel(client.ptid);
    }

    // for(int i = 0; i < bytes_received; i++)
    //   printf("%c", buffer[i]);
    // printf("\n");
    
    struct message *m = stringToMessage(buffer);
    printf("\n");
    fflush(stdout);

    if(m->type == JN_ACK){
      printf("Server: Joined %s", m->data);
      fflush(stdout);

      pthread_mutex_lock(&client_lock);
      client.currentSession = strdup((char *) m->data);
      pthread_mutex_unlock(&client_lock);
    } else{
      // printf("STRUCT CONTENTS2: %d %d %s %s\n", m->size, m->type, m->data, m->source);
      printf("%s", m->data);
      fflush(stdout);
    }

    printf("\n");
    fflush(stdout);

    free(m);
  }

}

void clientInputHandler(){
  char input[MAX_MESSAGE];

  while(1){
    // printf("> ");
    scanf("%[^\n]%*c", input);
    struct commandReturn *command = extractCommand(input);

    if(command->type == INVALID){
      printf("Error: Invalid command\n");
      free(command);
      continue;
    }
    
    int ret = performCommand(command);
    
    if(ret == LOGOUT)
      break;
  }
}
 
int main(int argc, char *argv[]){
  if(argc != 1)
    usage(argv[0]);  

  check_error(pthread_mutex_init(&client_lock, NULL), "client lock");

  while(1){
    startupClient();
    clientInputHandler();
  }
 
  return 0;
}