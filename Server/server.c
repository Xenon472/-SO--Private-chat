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
#define MAX_USERS 200
#define BUFFER_SIZE 2048
#define PORT 4444
#define HELP_STR "To send a message type the receiver, than two points ':', than the message\nTo show the list of who is online type 'list'\n\n"
#define OK_STRING "Succesfully logged in!\n\n"
#define FILE_NAME "database.txt"

static atomic_int clientNumber = 0;
static atomic_int usersNumber = 0;
static int uid = 10;
static char msgSeparator[] = ":";
static char bufSeparator[] = "|";
//user structure - la userò per il log-in
typedef struct userStr{
  char name[32];
  char surname[32];
  char username[32];
  char password[32];
}userStr;

//client struct
typedef struct clientStr{
  struct sockaddr_in address;
  int sockfd;
  int uid;
  int logged;
  struct userStr *user;
}clientStr;


FILE *db_file_ptr;
clientStr *clients[MAX_CLIENTS];
userStr *users[MAX_USERS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;


void my_exit(){
  //fclose(db_file_ptr);
  exit(0);
}

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

void send_online_list(int uid){
  pthread_mutex_lock(&clients_mutex);
  char listBuffer[((MAX_CLIENTS+2) * 32)+21];
  char name[34]; //32+2
  strcpy(listBuffer,"currently online: ");
  int sockfd;
  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(clients[i]->uid != uid){
		//printf("test logged -> %i\n",clients[i]->logged);
		if(clients[i]->logged == 1){
		  sprintf(name, "%s, ", clients[i]->user->username);
		  strcat(listBuffer, name);
	    }
      }
      else{
		sockfd = clients[i]->sockfd;
      }
    }
  }
  strcat(listBuffer, "\n");
  write(sockfd, listBuffer, strlen(listBuffer));
  bzero(listBuffer, ((MAX_CLIENTS+2) * 32));
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

int send_to(char *msg, char *dest){ //deprecated
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

int send_to_from(char *msg, char *dest, char *sender){
  int found = 0;
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(!strcmp(clients[i]->user->username, dest)){
	char buffer[BUFFER_SIZE + 32];
	sprintf(buffer, "%s->%s\n", sender, msg);
	write(clients[i]->sockfd, buffer, strlen(buffer));
	found = 1;
	break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  return found;
}

void send_users_list(clientStr *client){
  pthread_mutex_lock(&users_mutex);
  char listBuffer[((MAX_USERS+2) * 32)+21];
  char name[34]; //32+2
  strcpy(listBuffer,"users in database: ");
  for(int i=0; i<MAX_USERS; i++){
    if(users[i]){
	  sprintf(name, "%s, ", users[i]->username);
	  strcat(listBuffer, name);
    }
  }
  strcat(listBuffer, "\n");
  write(client->sockfd, listBuffer, strlen(listBuffer));
  bzero(listBuffer, ((MAX_USERS+2) * 32)+21);
  pthread_mutex_unlock(&users_mutex);
}

void populate_users_db(){
	//declaring buffer for getline
	int userCtr = 0;
	int usrSize = 134;
	char *line;
	line = (char*) malloc(usrSize*sizeof(char));
	size_t maxLineSize = usrSize;
	size_t currentLineSize;
	
	//file mutex
	pthread_mutex_lock(&file_mutex);  
	
	printf("test pre open\n");
    db_file_ptr = fopen (FILE_NAME, "r");
    printf("test post open\n");
    //if file does not exist, create it
    if(db_file_ptr == NULL){
	  db_file_ptr = fopen(FILE_NAME, "wb");
	  fclose(db_file_ptr);
	  db_file_ptr = fopen (FILE_NAME, "r");
	  printf("test creating file\n");
	}
	printf("test post create\n");
	
	while((currentLineSize = getline(&line, &maxLineSize, db_file_ptr)) != -1){
	  line[currentLineSize-1] = 0;
	  userStr *newUser = (userStr *)malloc(sizeof(userStr));
	  char *token = strtok(line,bufSeparator);
	  strcpy(newUser->username,token);
	  
	  token = strtok(NULL,bufSeparator);
	  strcpy(newUser->password,token);
	  
	  token = strtok(NULL,bufSeparator);
	  strcpy(newUser->name,token);
	  
	  token = strtok(NULL,bufSeparator);
	  strcpy(newUser->surname,token);
	  
	  users[userCtr] = newUser;
	  userCtr++;
	  usersNumber++;
	  bzero(line,usrSize);
	  if(usersNumber == MAX_USERS){
		  printf("ERROR: too many users in database!\n");
		  my_exit();	  
	  }
    }
    printf("test post read\n");
    
    fclose(db_file_ptr);
    free(line);
    pthread_mutex_unlock(&file_mutex);
}

void add_to_users_db(char *usr, char *psw, char *nme, char *sur){
  usersNumber++;
  char file_string[134]; //(32*4)+6
  sprintf(file_string, "%s|%s|%s|%s\n",usr,psw,nme,sur);
  	  
  //file mutex
  pthread_mutex_lock(&file_mutex);  
  
  //printf("test file string ->%s\n", file_string); 
  db_file_ptr = fopen (FILE_NAME, "a"); 
  fputs(file_string, db_file_ptr);
  fclose(db_file_ptr);
  
  pthread_mutex_unlock(&file_mutex);
  
  
  userStr *newUser;
  newUser = (userStr *)malloc(sizeof(userStr));
  
  strcpy(newUser->username , usr);
  strcpy(newUser->password , psw);
  strcpy(newUser->name , nme);
  strcpy(newUser->surname , sur);
  printf("Test newUser -> %s,%s,%s,%s\n",newUser->username,newUser->password,newUser->name,newUser->surname);
  //users mutex
  pthread_mutex_lock(&users_mutex);
  printf("Test Mutex!\n");
  
  for(int i=0; i<MAX_USERS; i++){
    if(!users[i]){
      users[i] = newUser;
      printf("Test users[%i] -> %s!\n",i,users[i]->username);
      break;
    }
  }  
  pthread_mutex_unlock(&users_mutex); 
  printf("Test CloseMutex!\n");
}

int username_already_taken(char *usr){
  int taken = 0; 
  printf("Test taken!\n"); 
  pthread_mutex_lock(&users_mutex);
  printf("Test Mutex taken!\n");
  
  for(int i=0; i<MAX_USERS; i++){
    if(users[i]){
	  printf("Test users[%i] -> %s!\n",i,users[i]->username);
      if(strcmp(users[i]->username, usr) == 0){		
		taken = 1;
		break;		
      }
    }
  }
  
  pthread_mutex_unlock(&users_mutex);
  printf("Test CloseMutex taken!\n");
  return taken;
}

/*int check_credentials(char *usr, char *psw){
  int found = 0;
  pthread_mutex_lock(&users_mutex);

  for(int i=0; i<MAX_USERS; i++){
    if(users[i]){
      if(strcmp(users[i]->username, usr) == 0){
		if(strcmp(users[i]->password, psw) == 0){
		  found = 1;
		  break;
		}
      }
    }
  }  
  pthread_mutex_unlock(&users_mutex);
  return found;
}*/

struct userStr * assign_user(char *usr){
  //use only when sure the user exist
  pthread_mutex_lock(&users_mutex);
  printf("Test Mutex assign!\n");
  
  int index;
  int flag = 0;
  for(int i=0; i<MAX_USERS; i++){
    if(users[i]){
	  printf("Test users[%i] -> %s!\n",i,users[i]->username);
      if(strcmp(users[i]->username, usr) == 0){
		index = i;
		flag = 1;
		break;
      }
    }
  }
  
  pthread_mutex_unlock(&users_mutex);
  printf("Test CloseMutex assign!\n");
  
  if(flag){
	return users[index];
  }
  return NULL;
}

int sign_client(clientStr *client){
  char uName[32];
  char password[32];
  char name[32];
  char surname[32];

  int uName_flag = 0;
  int password_flag = 0;
  int name_flag = 0;
  int surname_flag = 0;
  
  while(1){
	if(usersNumber == MAX_USERS){
	  printf("ERROR: max users in database reached!\n");
	  return 0;	  
	}
	if(!uName_flag){
	  send_to_uid("chose your username:\n",client->uid);
	  if(recv(client->sockfd, uName, 32, 0) <= 0){
	    printf("ERROR: RECEIVE.\n");
	    continue;
	  }
	  else if(strlen(uName) <  2 || strlen(uName) >= 32-1){
	    printf("ERROR: Invalid username.\n");
	    send_to_uid("ERROR: Invalid username.\n",client->uid);
	    bzero(uName, 32);
	    continue;
	  }
	  else if(username_already_taken(uName)){
	    printf("ERROR: username already taken.\n");
	    send_to_uid("ERROR: username already taken.\n",client->uid);
	    bzero(uName, 32);
	    continue;
	  }
	  else{
	    uName_flag = 1;
	  }
	}
	if(!password_flag){
	  send_to_uid("chose your password:\n",client->uid);
		if(recv(client->sockfd, password, 32, 0) <= 0){
		  printf("ERROR: RECEIVE.\n");
		  continue;
		}
	    else if(strlen(password) <  2 || strlen(password) >= 32-1){
		  printf("ERROR: Invalid password.\n");
		  send_to_uid("ERROR: Invalid password.\n",client->uid);
	      bzero(password, 32);
		  continue;
		}
		else{
		  password_flag = 1;
		}
	}
	if(!name_flag){
	  send_to_uid("enter your name:.\n",client->uid);
		if(recv(client->sockfd, name, 32, 0) <= 0){
		  printf("ERROR: RECEIVE.\n");
		  continue;
		}
		else if(strlen(name) <  2 || strlen(name) >= 32-1){
		  printf("ERROR: Invalid name.\n");
		  send_to_uid("ERROR: Invalid name.\n",client->uid);
		  bzero(name, 32);
		  continue;
		}
		else{
		  name_flag = 1;
		}
    }
	if(!surname_flag){
	  send_to_uid("enter your surname:\n",client->uid);
	  if(recv(client->sockfd, surname, 32, 0) <= 0){
		printf("ERROR: RECEIVE.\n");
	    continue;
	  }
	  else if(strlen(surname) <  2 || strlen(surname) >= 32-1){
		printf("ERROR: Invalid surname.\n");
		send_to_uid("ERROR: Invalid surname.\n",client->uid);
		bzero(surname, 32);
		continue;
	  }
      else{
		surname_flag = 1;
		break;
	  }
	}
  }
  add_to_users_db(uName,password,name,surname);
  client->user = assign_user(uName);
  return 1;
}

int log_in_db(clientStr *client){
	char logOrSign[8];
	int done_flag = 0;
	
	send_to_uid("Welcome! do you want to sign or log?\n",client->uid);
	
	while(1){
	  if(done_flag){
        break;
      }
      bzero(logOrSign, 8);
      //int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
      if(recv(client->sockfd, logOrSign, 8, 0) <= 0){
        printf("ERROR: RECEIVE.\n");
      }
      else if(strlen(logOrSign) <  2 || strlen(logOrSign) >= 8-1){
        printf("ERROR: Incorrect comand.\n");
        send_to_uid("ERROR: Incorrect comand.\n",client->uid);
      }
      else if(strcmp(logOrSign,"sign")==0 || strcmp(logOrSign,"Sign")==0){
        done_flag = sign_client(client);
      }
      else {
		printf("ERROR: Incorrect comand.\n");
        send_to_uid("ERROR: Incorrect comand.\n",client->uid);
	  }
    }
    return done_flag;
}

/*int log_in_file(clientStr *client){
  char uName[32];
  char password[32];
  char name[32];
  char surname[32]; 
  char logOrSign[8];
  int done_flag = 0;
  int logged_flag = 0;
  int uName_flag = 0;
  int password_flag = 0;
  int name_flag = 0;
  int surname_flag = 0;
  while(1){
	printf("ok1");
    if(done_flag){
      break;
    }
    //recv(client->sockfd, logOrSign, 8, 0);
    printf("'%s'\n",logOrSign);
    if(recv(client->sockfd, logOrSign, 8, 0) <= 0){
      printf("ERROR: RECEIVE.\n");
    }
    else if(strcmp(logOrSign, "sign") == 0 || strcmp(logOrSign, "Sign")){
      printf("ok2");
      while(1){
		  if(usersNumber == MAX_USERS){
		    printf("ERROR: max users in database reached!\n");
		    return 0;	  
		  }
		  if(uName_flag){
			send_to_uid("chose your username:\n",client->uid);
			if(recv(client->sockfd, uName, 32, 0) <= 0){
			  printf("ERROR: RECEIVE.\n");
			  continue;
			}
			else if(strlen(uName) <  2 || strlen(uName) >= 32-1){
			  printf("ERROR: Invalid username.\n");
			  send_to_uid("ERROR: Invalid username.\n",client->uid);
			  bzero(uName, 32);
			  continue;
			}
			else if(username_already_taken(uName)){
			  printf("ERROR: username already taken.\n");
			  send_to_uid("ERROR: username already taken.\n",client->uid);
			  bzero(uName, 32);
			  continue;
			}
			else{
			  uName_flag = 1;
			}
		  }
		  if(password_flag){
		  send_to_uid("chose your password:\n",client->uid);
			if(recv(client->sockfd, password, 32, 0) <= 0){
			  printf("ERROR: RECEIVE.\n");
			  continue;
			}
			else if(strlen(password) <  2 || strlen(password) >= 32-1){
			  printf("ERROR: Invalid password.\n");
			  send_to_uid("ERROR: Invalid password.\n",client->uid);
			  bzero(password, 32);
			  continue;
			}
			else{
			  password_flag = 1;
			}
		  }
		  if(name_flag){
			send_to_uid("enter your name:.\n",client->uid);
			if(recv(client->sockfd, name, 32, 0) <= 0){
			  printf("ERROR: RECEIVE.\n");
			  continue;
			}
			else if(strlen(name) <  2 || strlen(name) >= 32-1){
			  printf("ERROR: Invalid name.\n");
			  send_to_uid("ERROR: Invalid name.\n",client->uid);
			  bzero(name, 32);
			  continue;
			}
			else{
			  name_flag = 1;
			}
		  }
		  if(surname_flag){
			send_to_uid("enter your surname:.\n",client->uid);
			if(recv(client->sockfd, surname, 32, 0) <= 0){
			  printf("ERROR: RECEIVE.\n");
			  continue;
			}
			else if(strlen(surname) <  2 || strlen(surname) >= 32-1){
			  printf("ERROR: Invalid surname.\n");
			  send_to_uid("ERROR: Invalid surname.\n",client->uid);
			  bzero(surname, 32);
			  continue;
			}
			else{
			  surname_flag = 1;
			  break;
			}
		  }	  
	  }	  

	  add_to_users_db(uName,password,name,surname);

	  client->user = assign_user(uName);
	  done_flag = 1;
    }
    else if(strcmp(logOrSign, "log") == 0 || strcmp(logOrSign, "login") || strcmp(logOrSign, "Log") || strcmp(logOrSign, "Login")){
		while(1){
		  if(logged_flag){
				done_flag = 1;
				break;
		  }
		  if(uName_flag){
			send_to_uid("enter your username:\n",client->uid);
			if(recv(client->sockfd, uName, 32, 0) <= 0){
			  printf("ERROR: RECEIVE.\n");
			  continue;
			}
			else if(strlen(uName) <  2 || strlen(uName) >= 32-1){
			  printf("ERROR: Invalid username.\n");
			  send_to_uid("ERROR: Invalid username.\n",client->uid);
			  bzero(uName, 32);
			  continue;
			}
			else{
			  uName_flag = 1;
			}
		  }
		  if(password_flag){
			send_to_uid("enter your password:\n",client->uid);
			if(recv(client->sockfd, password, 32, 0) <= 0){
			  printf("ERROR: RECEIVE.\n");
			  continue;
			}
			else if(strlen(password) <  2 || strlen(password) >= 32-1){
			  printf("ERROR: Invalid password.\n");
			  send_to_uid("ERROR: Invalid password.\n",client->uid);
			  bzero(password, 32);
			  continue;
			}
			else{
			  password_flag = 1;
			}
		  }
		  if(check_credentials(uName,password)){
			  client->user = assign_user(uName);
			  logged_flag = 1;
			  break;
		  }
		  else{
			  printf("ERROR: incorrect username or password.\n");
			  send_to_uid("ERROR: incorrect username or password.\nretry.\n",client->uid);
			  uName_flag = 0;
			  password_flag = 0;
			  bzero(uName, 32);
			  bzero(password, 32);
		  } 
		}
	}
	else{
	  printf("ERROR: choose login or signup.\n");
	  send_to_uid("ERROR: choose login or signup.\n",client->uid);
	}
  }
  return 1;
}*/

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
      //send_to_all(buffer, client->uid); //not needed after list comand
      done_flag = 1;
    }
    bzero(buffer, BUFFER_SIZE);
  }
  return 1;
}

void *handle_client(void *arg){
  printf("test handle client\n");
  clientStr *client = (clientStr*)arg;
  //client->user = (userStr *)malloc(sizeof(userStr));
  //userStr *currentUser = (userStr *)malloc(sizeof(userStr));
  //client->user = currentUser;
  
  int logInFlag = log_in_db(client);
  int connected_flag = 0;
  char buffer[BUFFER_SIZE];
  char msgBuffer[BUFFER_SIZE];
  char destBuffer[32]; 

  if(logInFlag){
    connected_flag = 1;
    client->logged = 1;
    send_to_uid("Successdully logged in!\n",client->uid);
  }
  clientNumber++;
  while(1){
    if(connected_flag != 1){
      break;
    }
    bzero(destBuffer, 32);
    //Receive:
    int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
    if (receive > 0){      
      if(strlen(buffer) > 0){
	char *ptr = strtok(buffer, msgSeparator);
	if(strlen(ptr) < 32){
	  printf("\ntest ptr->%s\n",ptr); //test
	  strcpy(destBuffer, ptr);
	  printf("\ntest->%s\n",destBuffer); //test
	  if(strcmp(destBuffer, "exit") == 0 || strcmp(destBuffer, "exit\n") == 0 ){
	    sprintf(msgBuffer, "%s has left\n", client->user->username);
	    printf("%s", msgBuffer);
	    send_to_all(msgBuffer, client->uid);
	    connected_flag = 0;
	    break;
	    //continue;
	  }
	  else if(strcmp(destBuffer, "help") == 0 || strcmp(destBuffer, "help\n") == 0 ){
	    sprintf(msgBuffer, HELP_STR);
	    send_to_uid(msgBuffer, client->uid);
	    bzero(ptr,32);
	    continue;
	  }
	  else if(strcmp(destBuffer, "list") == 0 || strcmp(destBuffer, "list\n") == 0 ){
	    send_online_list(client->uid);
	    bzero(ptr,32);
	    continue;
	  }
	  else if(strcmp(destBuffer, "users") == 0 || strcmp(destBuffer, "users\n") == 0 ){
	    send_users_list(client);
	    bzero(ptr,32);
	    continue;
	  }
	}
	else{
	  send_to_uid("ERROR: invalid receiver\n",client->uid);
	  bzero(ptr,32);
	  continue;
	}
	//printf("'%s'\n", ptr);  //test
	ptr = strtok(NULL, "");
	//printf("'%s'\n", ptr);	//test
	if(ptr==NULL){
	  send_to_uid("ERROR: invalid formatting\n",client->uid);
	  bzero(ptr,32);
	  continue;
	}
	strcpy(msgBuffer, ptr);
	//printf("'%s'\n", msgBuffer); //test
	if(strlen(msgBuffer) > 0){
	  if(send_to_from(msgBuffer,destBuffer,client->user->username) == 0){
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
    
    bzero(buffer, BUFFER_SIZE);
    bzero(msgBuffer, BUFFER_SIZE);
    bzero(destBuffer, 32);
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
  if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
	  printf("ERROR: pthread_create.\n");
	  exit(0);
  }
  bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(listenfd, 10);  

  //server active
  printf("---SERVER ACTIVE---\n");

  //opening database file
  populate_users_db();
  printf("test post populte\n");
  //overwriting ctrl+c  
  signal(SIGINT, my_exit);

  //main loop
  while(1){
	printf("test while\n");
    socklen_t cliLen = sizeof(client_addr);
    printf("test socklen\n");
    connfd = accept(listenfd,(struct sockaddr*)&client_addr, &cliLen);
	printf("test connection\n");
    //check client numbers,	
	if(clientNumber < MAX_CLIENTS){
		printf("test if\n");
		//setting up client
		clientStr *client = (clientStr *)malloc(sizeof(clientStr));
		client->address = client_addr;
		client->sockfd = connfd;
		client->uid = uid++;
		client->logged = 0;

		add_client(client);
		printf("test pre handle client\n");
		if(pthread_create(&tid, NULL, &handle_client, (void*)client) != 0){
		  printf("ERROR: pthread_create.\n");
		}
	}
    
    sleep(1);    
  }

  fclose(db_file_ptr);
  return EXIT_SUCCESS;
}
