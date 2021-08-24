#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048
#define USER_DATA_LENGTH 32
#define PORT 4444

#define stdin_clean(stdin) while ((getchar()) != '\n')

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char uName[USER_DATA_LENGTH];

void ctrl_c_exit(){
  flag = 1;
  exit(0);
}

void msg_trim (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void login(){  
  while(1){    
    printf("Enter your username: ");
    fgets(uName, USER_DATA_LENGTH, stdin);
    msg_trim(uName, strlen(uName));
    
    if (strlen(uName) > USER_DATA_LENGTH - 2 || strlen(uName) < 2){
      printf("Name must be less than 30 and more than 2 characters.\n");
      if(strlen(uName) > USER_DATA_LENGTH - 2){
	stdin_clean(stdin);
      }
    }
    else{
      break;
    }
  }
}

int main(int argc, char **argv){

  char *ip = "127.0.0.1";

  signal(SIGINT, ctrl_c_exit);

  //printf("%s",uName);
  struct sockaddr_in server_addr;

  //Socket settings
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(PORT);
  server_addr.sin_family = AF_INET;

  //connecting client to server
  if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
    printf("ERROR: connection failed!\n");
    return EXIT_FAILURE;
  }
    
  login();
  send(sockfd, uName, 32, 0);
  printf("CONNECTED!\n");
  
  
}
