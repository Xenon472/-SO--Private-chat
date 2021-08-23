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
static char separator[] = ":";

//user structure - la userò per il log-in
typedef struct userStr{
  char name[32];
  char surname[32];
  char username[32];
  char Password[32];
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

void send_to_all(char *msg, int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(clients[i]->uid != uid){
	write(clients[i]->sockfd, msg, strlen(msg));
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void send_to_uid(char *msg, int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(clients[i]->uid == uid){
	write(clients[i]->sockfd, msg, strlen(msg));
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

int send_to(char *msg, char *dest){
  int found = 0;
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(!strcmp(clients[i]->user->username, dest)){
	write(clients[i]->sockfd, msg, strlen(msg));
	found = 1;
	break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  return found;
}

int log_in(clientStr *client){  
  char buffer[BUFFER_SIZE];
  char uName[32];
  int done_flag = 0;
  while(1){
    if(done_flag){
      break;
    }
    bzero(uName, 32);
    //int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
    if(recv(client->sockfd, uName, 32, 0) <= 0){
      printf("ERROR: RECEIVE.\n");
    }
    else if(strlen(uName) <  2 || strlen(uName) >= 32-1){
      printf("ERROR: Incorrect username.\n");
      send_to_uid("ERROR: Incorrect username.\n",client->uid);
    }
    else{

      strcpy(client->user->username, uName);

      sprintf(buffer, "%s has joined\n", client->user->username);
      printf("%s", buffer);
      send_to_all(buffer, client->uid);
      done_flag = 1;
    }
    bzero(buffer, BUFFER_SIZE);
  }
  return 1;
}

void *handle_client(void *arg){
  
  clientStr *client = (clientStr*)arg;
  client->user = (userStr *)malloc(sizeof(userStr));
  //userStr *currentUser = (userStr *)malloc(sizeof(userStr));
  //client->user = currentUser;
  
  int logInFlag = log_in(client);
  int connected_flag = 0;
  char buffer[BUFFER_SIZE];
  char msgBuffer[BUFFER_SIZE];
  char destBuffer[32]; 

  if(logInFlag){
    connected_flag = 1;
  }
  clientNumber++;
  while(1){
    if(connected_flag != 1){
      break;
    }
    //Receive:
    int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
    if (receive > 0){      
      if(strlen(buffer) > 0){
	char *ptr = strtok(buffer, separator);
	if(strlen(ptr) < 32){
	  strcpy(destBuffer, ptr);
	  printf("\ntest->%s\n",destBuffer);
	  if(strcmp(destBuffer, "exit") == 0 || strcmp(destBuffer, "exit\n") == 0 ){
	    //printf("AAAAARGHHHHHH");
	    sprintf(msgBuffer, "%s has left\n", client->user->username);
	    printf("%s", msgBuffer);
	    send_to_all(msgBuffer, client->uid);
	    connected_flag = 0;
	    break;
	  }
	}
	else{
	  send_to_uid("ERROR: invalid receiver\n",client->uid);
	  continue;
	}
	//printf("'%s'\n", ptr);  //test
	ptr = strtok(NULL, "");
	//printf("'%s'\n", ptr);	//test
	if(ptr==NULL){
	  send_to_uid("ERROR: invalid formatting\n",client->uid);
	  continue;
	}
	strcpy(msgBuffer, ptr);
	printf("'%s'\n", msgBuffer); //test
	if(strlen(msgBuffer) > 0){
	  if(send_to(msgBuffer,destBuffer) == 0){
	    send_to_uid("ERROR: could not find receiver\n",client->uid);
	    //printf("\n%s->%s:%s\n",client->user->username,destBuffer,msgBuffer);
	    printf("message sent: %s->%s:%s\n", client->user->username, destBuffer, msgBuffer);
	  }
	}
      }
    }
    else if (receive == 0){
      sprintf(msgBuffer, "%s has left\n", client->user->username);
      printf("%s", msgBuffer);
      send_to_all(msgBuffer, client->uid);
      connected_flag = 0;
    }
    else {
      printf("ERROR: -1\n");
      connected_flag = 0;
    }
    //vari if ecc --TODO--
    send_to_all(msgBuffer, client->uid);

    
    
    bzero(buffer, BUFFER_SIZE);
    bzero(msgBuffer, BUFFER_SIZE);
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
