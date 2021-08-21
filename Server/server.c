#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <stdatomic.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define PORT 4444

static atomic_int clientNumber = 0;
static int uid = 10;

//user structure - la userò per il log-in
typedef struct userStr{
  char *name;
  char *surname;
  char *username;
  char *Password;
}userStr;

//client struct
typedef struct clientStr{
  struct sockaddr_in address;
  int sockfd;
  int uid;
  int logged;
  struct userStr *user;
}clientStr;



clientStr *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(clientStr *client){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(!clients[i]){
      clients[i] = client;
      break;
    }
  }  
  pthread_mutex_unlock(&clients_mutex);
}

void remove_client(clientStr *client){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(clients[i]->uid == client->uid){
	clients[i] = NULL;
        break;
      }
    }
  }  
  pthread_mutex_unlock(&clients_mutex);
}

void send_to_all(char *s, int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(clients[i]->uid != uid){
	write(clients[i]->sockfd, s, strlen(s));
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
  
  clientStr *client = (clientStr*)arg;
  client->user = (userStr *)malloc(sizeof(userStr));
  //userStr *currentUser = (userStr *)malloc(sizeof(userStr));
  //client->user = currentUser;
  //login --TODO--
  
  //after log-in
  char buffer[BUFFER_SIZE];  
  int connected_flag = 1;
  clientNumber++;
  while(1){
    if(!connected_flag){
      break;
    }
    int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
    //vari if ecc --TODO--
    send_to_all(buffer, client->uid);
    printf("message sent: %s", buffer);

    bzero(buffer, BUFFER_SIZE);

  }
  close(client->sockfd);
  remove_client(client);
  free(client->user);
  free(client);
  clientNumber--;
  pthread_detach(pthread_self());

  return NULL;
}

int main(void){
  //variables
  int option = 1;
  int listenfd = 0;
  int connfd = 0;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  pthread_t tid;

  //socket
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  server_addr.sin_port = htons(PORT);
  server_addr.sin_family = AF_INET;

  //Signals
  signal(SIGPIPE, SIG_IGN);
  
  //bind & listen
  setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option));
  bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(listenfd, 10);  

  //server active
  printf("---SERVER ACTIVE---\n");

  //main loop
  while(1){
    socklen_t cliLen = sizeof(client_addr);
    connfd = accept(listenfd,(struct sockaddr*)&client_addr, &cliLen);

    //check client numbers, --Todo!--

    //setting up client
    clientStr *client = (clientStr *)malloc(sizeof(clientStr));
    client->address = client_addr;
    client->sockfd = connfd;
    client->uid = uid++;
    client->logged = 0;

    // LOG IN --TODO--

    add_client(client);
    pthread_create(&tid, NULL, &handle_client, (void*)client);

    //
    sleep(1);    
  }

    
  return EXIT_SUCCESS;
}
