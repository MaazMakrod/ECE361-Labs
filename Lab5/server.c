#include "common.h"

struct clientNode *clients;
int clientsLen = 0;
char **sessions;

pthread_mutex_t clients_lock;
pthread_mutex_t session_lock;

void setupGlobalStructures();
static void usage(char *program);
int login(struct message *m, int client_fd);
void *clientHandler(void *socket_fd);
int performAction(struct message *m);
int exitClient(struct message *m);
int query(struct message *m);
int getClientIndex(struct message *m);
int createSession(struct message *m);
int joinSession(struct message *m);
int leavesession(struct message *m);
int message(struct message *m);
int directMessage(struct message *m);
int invite(struct message *m);

int login(struct message *m, int client_fd){
  int clientExists = FALSE;
  int correctPass = FALSE;
  int clientAlreadyInUse = FALSE;
  int i;

  // printf("%d\n%s\n%d\n%s\n\n", m->type, m->source, m->size, m->data); 

  pthread_mutex_lock(&clients_lock); 

  for(i = 0; i < clientsLen; i++){
    if(strcmp(clients[i].name, (char *) m->source) == 0){
        clientExists = TRUE;

        if(strcmp(clients[i].password, (char *) m->data) == 0){

          if(clients[i].loggedIn){
            clientAlreadyInUse = TRUE;
            break;
          }
          
          correctPass = TRUE;
          clients[i].loggedIn = TRUE;
          clients[i].socket_fd = client_fd;
        }

        break;
    }
  }

  pthread_mutex_unlock(&clients_lock);

  struct message *r = malloc(sizeof(struct message));
  memcpy(&(r->source), "server", strlen("server"));
  r->size = 0;

  if(correctPass && clientExists && !clientAlreadyInUse){
    r->type = LO_ACK;
  } else {
    r->type = LO_NAK;

    if (clientAlreadyInUse){
      memcpy(&(r->data), "Error: Client is already in use", strlen("Error: Client is already in use"));
      r->size = strlen("Error: Client is already in use");
    } else if(clientExists){
      memcpy(&(r->data), "Error: Password is incorrect", strlen("Error: Password is incorrect"));
      r->size = strlen("Error: Password is incorrect");
    } else {
      memcpy(&(r->data), "Error: User does not exist", strlen("Error: User does not exist"));
      r->size = strlen("Error: User does not exist");
    }
  }
  
  char *res = messageToString(r);
  check_error(send(client_fd, res, strlen(res), 0), "login response");
  free(res);
  free(r);

  return (correctPass && clientExists) ? i : -1;
}

static void usage(char *program) {
	fprintf(stderr, "Usage: %s <port_number> \n", program);
	exit(1);
}

void setupGlobalStructures(){
  FILE *ptr = fopen("./clients.txt", "r");
  
  char* line = NULL;
  size_t len = 0;
  int count = 0;

  while(getline(&line, &len, ptr) != -1){
    line[strcspn(line, "\n")] = 0;

    if(count == 0){
      clientsLen = atoi(line);
      clients = (struct clientNode *) malloc(sizeof(struct clientNode)*clientsLen);
    } else {
      clients[count-1].loggedIn = FALSE;
      clients[count-1].currentSession = NULL;
      clients[count-1].socket_fd = -1;

      char *param = strtok(line, ",");
      clients[count-1].name = strdup(param);
      param = strtok(NULL, ",");
      clients[count-1].password = strdup(param);     
    }
    count++;
  }

  fclose(ptr);

  sessions = (char **) malloc(sizeof(char *)*MAX_SESSIONS);

  for(int i = 0; i < MAX_SESSIONS; i++)
    sessions[i] = NULL;

  check_error(pthread_mutex_init(&clients_lock, NULL), "clients lock");
  check_error(pthread_mutex_init(&session_lock, NULL), "session lock");
}

int getClientIndex(struct message *m){
  int index = -1;

  for(int i = 0; i < clientsLen; i++){
    if(strcmp(clients[i].name, (char *) m->source) == 0){
      index = i;
      break;
    }
  }

  return index;
}

int exitClient(struct message *m){
  int i = getClientIndex(m);
  assert(i != -1);//Client must be found

  pthread_mutex_lock(&clients_lock);

  printf("Exiting client %s\n", clients[i].name);

  if(clients[i].currentSession != NULL)
    free(clients[i].currentSession);

  clients[i].loggedIn = FALSE;
  close(clients[i].socket_fd);
  
  pthread_mutex_unlock(&clients_lock);

  pthread_exit(NULL);

  return 1;
}

int createSession(struct message *m){
  // printMessage(m);
  int index = getClientIndex(m);
  assert(index != -1);
  int i;
  int canInsert = FALSE;

  pthread_mutex_lock(&session_lock);

  for(i = 0; i < MAX_SESSIONS; i++){
    if(sessions[i] == NULL){
      canInsert = TRUE;
      break;
    }
  }

  if(canInsert){
    // sessions[i] = (char *) malloc(m->size);
    // memcpy(sessions[i], m->data, m->size);
    sessions[i] = strdup((char*)m->data);
    // printf("%s - \n", m->data);
    printf("Created session %s\n", sessions[i]);
  }

  pthread_mutex_unlock(&session_lock);
  
  free(m);

  struct message *r = (struct message *) malloc(sizeof(struct message));
  bzero(r->source, MAX_NAME);
  bzero(r->data, MAX_DATA);

  r->type = canInsert ? NS_ACK : NS_NAK;
  
  if(canInsert){
    r->size = strlen("Server: Created session");
    memcpy(&(r->source), "server", strlen("server"));
    memcpy(&(r->data), "Server: Created session", strlen("Server: Created session"));
  } else {
    r->size = strlen("Cannot create more sessions at this time");
    memcpy(&(r->source), "server", strlen("server"));
    memcpy(&(r->data), "Cannot create more sessions at this time", strlen("\nCannot create more sessions at this time"));
  }

  char *message = messageToString(r);

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);
  free(r);

  // printf("%s\n", message);
  check_error(send(clients[index].socket_fd, buffer, sizeof(buffer), 0), "sending to exit to server");  

  return NEW_SESS;
}

int joinSession(struct message *m){
  int sessionExists = FALSE;
  int index = getClientIndex(m);
  assert(index != -1);

  pthread_mutex_lock(&session_lock);

  for(int i = 0; i < MAX_SESSIONS; i++){
    if(sessions[i] != NULL && strcmp(sessions[i], (char*) m->data) == 0){
      sessionExists = TRUE;
      break;
    }
  }

  pthread_mutex_unlock(&session_lock);

  if(sessionExists) {
    pthread_mutex_lock(&clients_lock);
    
    if(clients[index].currentSession != NULL){
      free(clients[index].currentSession);
      clients[index].currentSession = NULL;
    }

    clients[index].currentSession = strdup((char*)m->data);

    pthread_mutex_unlock(&clients_lock);
  }

  struct message *r = (struct message *) malloc(sizeof(struct message));
  bzero(r->source, MAX_NAME);
  bzero(r->data, MAX_DATA);
  memcpy(&(r->source), "server", strlen("server"));
  r->type = sessionExists ? JN_ACK : JN_NAK;

  if(sessionExists){
    r->size = strlen(clients[index].currentSession);
    memcpy(&(r->data), clients[index].currentSession, strlen(clients[index].currentSession));
  } else {
    char *data = (char *) malloc(MAX_DATA);
    bzero(data, MAX_DATA);

    strcat(data, (char *)m->data);
    strcat(data, " is not a valid session");

    r->size = strlen(data);
    memcpy(&(r->data), data, strlen(data));

    free(data);
  }

  char *message = messageToString(r);

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  strcpy(buffer, message);
  free(message);
  free(r);

  // printf("%s\n", message);
  check_error(send(clients[index].socket_fd, buffer, sizeof(buffer), 0), "sending to exit to server");

  free(m);
  return JN_ACK;
}

int leavesession(struct message *m){
  int index = getClientIndex(m);
  assert(index != -1);
  free(m);

  int clientCount = 0;

  pthread_mutex_lock(&clients_lock);

  for(int i = 0; i < clientsLen; i++){
    if(clients[i].loggedIn && clients[i].currentSession != NULL && strcmp(clients[index].currentSession, clients[i].currentSession) == 0){
      clientCount++;
      if(clientCount == 2)
        break;
    }
  }

  pthread_mutex_unlock(&clients_lock);

  if(clientCount == 1){
    pthread_mutex_lock(&session_lock);

    for(int i = 0; i < MAX_SESSIONS; i++){
      if(sessions[i] != NULL && strcmp(clients[index].currentSession, sessions[i]) == 0){
        free(sessions[i]);
        sessions[i] = NULL;
        break;
      }
    }

    pthread_mutex_unlock(&session_lock);
  }

  free(clients[index].currentSession);
  clients[index].currentSession = NULL;

  return LEAVE_SESS;
}

int message(struct message *m){
  // printf("MESSAGE: %s\n", m->data);
  // printMessage(m);

  int index = getClientIndex(m);
  struct message *r = (struct message *) malloc(sizeof(struct message));
  bzero(r->source, MAX_NAME);
  bzero(r->data, MAX_DATA);
  memcpy(&(r->source), "server", strlen("server"));

  r->type = MESSAGE;

  char *data = (char *) malloc(MAX_DATA);
  bzero(data, MAX_DATA);

  strcat(data, (char *)m->source);
  strcat(data, ": ");
  strcat(data, (char *)m->data);

  r->size = strlen(data);
  memcpy(&(r->data), data, strlen(data));
  free(data);

  char *message = messageToString(r); 
  free(r);
  
  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);

  pthread_mutex_lock(&clients_lock);

  for(int i = 0; i < clientsLen; i++){
    if(clients[i].loggedIn && clients[i].currentSession != NULL && strcmp(clients[i].currentSession, clients[index].currentSession) == 0){
      if(i != index)
        check_error(send(clients[i].socket_fd, buffer, sizeof(buffer), 0), "sending message");
    }
  }

  pthread_mutex_unlock(&clients_lock);
  free(m);

  return MESSAGE;
}

int directMessage(struct message* m){
  // printf("%s %s\n", m->source ,m->data);
  int source_index = getClientIndex(m);
  char *save_ptr;

  char *user_id = strtok_r((char *) m->data, "^", &save_ptr);
  char *dm = strtok_r(NULL, "", &save_ptr);

  // printf("%s %s\n", user_id, dm);

  int send_index = -1;

  for(int i = 0; i < clientsLen; i++){
    if(strcmp(clients[i].name, user_id) == 0){
      send_index = i;
      break;
    }
  }

  int send_socket = clients[source_index].socket_fd;

  struct message *r = (struct message *) malloc(sizeof(struct message));
  bzero(r->source, MAX_NAME);
  bzero(r->data, MAX_DATA);
  memcpy(&(r->source), "server", strlen("server"));
  r->type = DM;

  if(send_index == -1){
    memcpy(&(r->data), "Error: user does not exist", strlen("Error: user does not exist"));
    r->size = strlen("Error: user does not exist");
  } else if (send_index == source_index) {
    memcpy(&(r->data), "Error: cannot send dm to yourself", strlen("Error: cannot send dm to yourself"));
    r->size = strlen("Error: cannot send dm to yourself");
  } else if (clients[send_index].loggedIn == FALSE){
    memcpy(&(r->data), "Error: user is not logged in", strlen("Error: user is not logged in"));
    r->size = strlen("Error: user is not logged in");
  } else {
    memcpy(&(r->data), dm, strlen(dm));
    r->size = strlen(dm);
    send_socket = clients[send_index].socket_fd;
  }

  char *message = messageToString(r); 
  free(r);

  // printf("%s | %d\n", message, send_socket);
  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);  

  check_error(send(send_socket, buffer, sizeof(buffer), 0), "sending dm res");

  free(m);
  return DM;
}

int invite (struct message *m){
  // printf("Invitation was received\n");
  char *save_ptr;

  char *session_id = strtok_r((char *) m->data, "^", &save_ptr);
  char *user_id = strtok_r(NULL, "^", &save_ptr);
  char *sent_message = strtok_r(NULL, "", &save_ptr);

  int source_index = getClientIndex(m);

  // printf("DATA: %s %s %s\n", session_id, user_id, message);
  // printf("here1\n");

  int send_index = -1;

  for(int i = 0; i < clientsLen; i++){
    if(strcmp(clients[i].name, user_id) == 0){
      send_index = i;
      break;
    }
  }

// printf("here2\n");

  int send_socket = clients[source_index].socket_fd;

  struct message *r = (struct message *) malloc(sizeof(struct message));
  bzero(r->source, MAX_NAME);
  bzero(r->data, MAX_DATA);
  memcpy(&(r->source), "server", strlen("server"));
  r->type = MESSAGE;

  // printf("here3\n");

  if(send_index == -1){
    memcpy(&(r->data), "Error: user does not exist", strlen("Error: user does not exist"));
    r->size = strlen("Error: user does not exist");
  } else if (send_index == source_index) {
    memcpy(&(r->data), "Error: cannot send invite to yourself", strlen("Error: cannot send invite to yourself"));
    r->size = strlen("Error: cannot send invite to yourself");
  } else if (clients[send_index].loggedIn == FALSE){
    memcpy(&(r->data), "Error: user is not logged in", strlen("Error: user is not logged in"));
    r->size = strlen("Error: user is not logged in");
  } else {
    r->type = INVITE;
    char *invite_message = malloc(MAX_DATA);

    strcpy(invite_message, session_id);
    strcat(invite_message, "^");
    strcat(invite_message, sent_message);

    memcpy(&(r->data), invite_message, strlen(invite_message));
    r->size = strlen(invite_message);
    send_socket = clients[send_index].socket_fd;

    free(invite_message);
  }

  // printf("here4\n");

  char *message = messageToString(r); 
  free(r);

  // printf("%s | %d\n", message, send_socket);
  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  memcpy(buffer, message, MAX_MESSAGE);
  free(message);  

  // printf("here5\n");

  check_error(send(send_socket, buffer, sizeof(buffer), 0), "sending invite res");

  free(m);
  // printf("here6\n");
  return INVITE;
}

int performAction(struct message *m){  
  assert(m->type != LOGIN); //Client should already be logged in

  switch(m->type){
    case MESSAGE:
      return message(m);//printf("The MESSAGE command was sent\n");
      break;
    case EXIT:
      return exitClient(m);
      break;
    case LOGOUT:
      printf("The LOGOUT command was sent\n");
      break;
    case JOIN:
      return joinSession(m);//printf("The JOIN command was sent\n");
      break;
    case NEW_SESS:
      return createSession(m);// printf("The NEW_SESS command was sent\n");
      break;
    case LEAVE_SESS:
      return leavesession(m);// printf("The LEAVE_SESS command was sent\n");
      break;
    case QUERY:
      return query(m);// printf("The QUERY command was sent\n"); 
      break;
    case DM:
      return directMessage(m);
      break;
    case INVITE:
      return invite(m);
      break;
  }
    
    free(m);

  return 1;
}

void *clientHandler(void *socket_fd) {
  char buffer[MAX_MESSAGE];
  int client_fd = *((int*) socket_fd);

  while(1){
    bzero(buffer, MAX_MESSAGE);
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    check_error(bytes_received, "recv from client");

    
    if(bytes_received == 0){
      struct message *m = malloc(sizeof(struct message));
      bzero(m->data, MAX_DATA);
      bzero(m->source, MAX_NAME);
      m->type = EXIT;
      
      for(int i = 0; i < clientsLen; i++){
        if(clients[i].loggedIn && clients[i].socket_fd == client_fd){
          memcpy(&(m->source), clients[i].name, strlen(clients[i].name));
          break;
        }
      }

      performAction(m);
    }

    // for(int i = 0; i < bytes_received; i++)
    //   printf("%c", buffer[i]);
    // printf("\n");

    struct message* m = stringToMessage(buffer);
    performAction(m);
  }
}

int query(struct message *m){
  int index = getClientIndex(m);
  assert(index != -1);//Client must be found
  free(m);

  struct message *r = malloc(sizeof(struct message));
  bzero(r->data, MAX_DATA);
  bzero(r->source, MAX_NAME);

  memcpy(&(r->source), "server", strlen("server"));
  r->type = QU_ACK;

  char *data = malloc(MAX_DATA);
  bzero(data, MAX_DATA);
  strcat(data, "Current active clients:\n");

  pthread_mutex_lock(&clients_lock);

  for(int i = 0; i < clientsLen; i++){
    if(clients[i].loggedIn){
      strcat(data, clients[i].name);

      if(clients[i].currentSession != NULL){
        strcat(data, " - ");
        strcat(data, clients[i].currentSession);
      }

      strcat(data, "\n");
    }
  }

  pthread_mutex_unlock(&clients_lock);

  strcat(data, "\nCurrent active sessions:\n");

  pthread_mutex_lock(&session_lock);

  for(int i = 0; i < MAX_SESSIONS; i++){
    if(sessions[i] != NULL){
      strcat(data, (char *) sessions[i]);
      strcat(data, "\n");
    }
  }

  pthread_mutex_unlock(&session_lock);

  r->size = strlen(data);
  memcpy(&(r->data), data, r->size);
  free(data);

  char buffer[MAX_MESSAGE];
  bzero(buffer, MAX_MESSAGE); 
  char *message = messageToString(r);

  memcpy(buffer, message, MAX_MESSAGE);
  free(message);

  check_error(send(clients[index].socket_fd, buffer, sizeof(buffer), 0), "sending to client");
  free(r);

  return QUERY;
}
 
int main(int argc, char *argv[]){
  if(argc != 2)
    usage(argv[0]);

  int port = atoi(argv[1]);
  int socket_fd;

  if(port < 1024) {
    fprintf(stderr, "port = %d, should be >= 1024\n", port);
    usage(argv[0]);
  }

  setupGlobalStructures();

  struct addrinfo hints, *server_info, *p;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  check_error(getaddrinfo(NULL, argv[1], &hints, &server_info), "getaddrinfo");

  for(p = server_info; p != NULL; p = p->ai_next){
    if(( socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol) ) == -1){
      printf("socket fail\n");
      continue;
    }
    
    if(bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
      printf("bind fail\n");
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

  //Listen to incoming connections (max 10)
  check_error(listen(socket_fd, 10), "listen");

  struct sockaddr_in client_info;
  socklen_t addrlen = sizeof(client_info);
  int client_fd;
  char buffer[MAX_MESSAGE];

  // printf("Waiting for client connections...\n\n");

  //Main thread listens for client connections and permits them if client exists
  while(1){
    printf("Waiting for client connections...\n\n");

    client_fd = accept(socket_fd, (struct sockaddr*) &client_info, &addrlen);
    check_error(client_fd, "accept client");
    printf("Accepting client %d \n", client_fd);
    
    bzero(buffer, MAX_MESSAGE);
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    check_error(bytes_received, "recv from client");

    struct message* m = stringToMessage(buffer);
    int ret = login(m, client_fd);
    free(m);

    if( ret != -1 && clients[ret].loggedIn){
      printf("Logged in client\n");
      pthread_create(&clients[ret].ptid, NULL, &clientHandler, &client_fd);      
      pthread_detach(clients[ret].ptid);
    } else {
      close(client_fd);
      printf("Client disconnected.\n\n");
    }
  }

  return 0;
}