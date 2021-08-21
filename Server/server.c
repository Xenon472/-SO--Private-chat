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

#define MAX_CLIENTS 100
#define BUFFER 2048
#define PORT 4444

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
  printf("---SERVER ACTIVE---");

    
  return EXIT_SUCCESS;
}
