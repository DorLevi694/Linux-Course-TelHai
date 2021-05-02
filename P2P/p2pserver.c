#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "p2p.h"
#include <stdlib.h>
#include <pthread.h>

int startMainServer(struct sockaddr_in *myAddrass);
int takeCareNewSocket(int curSockt,int curClientNum);
void getNewFileToSystem(int curSockt);
void* runNewClientThread(void* new_socket);
void sendAllFilesOnSystem(int curSockt);
pthread_mutex_t  mutex;
int port = P_SRV_PORT +1;
int numOfFiles=0;
file_ent_t allFiles[MAX_FILES_NUM];
int clientNum = 0;

int main(int argc,char* argv[]){
  pthread_t pthread;
  int curClientNum=clientNum;
  int socket_fd,new_socket,msg_type;
  struct sockaddr_in myAddrass;
  socket_fd = startMainServer(&myAddrass);
  pthread_mutex_init( &mutex, NULL );

  while(curClientNum < MAX_CLIENT_NUM){
    new_socket = accept(socket_fd, (struct sockaddr*)NULL,(socklen_t*)NULL);
   if (new_socket == 0){
      printf("accept failed\n");
      exit(EXIT_ERROR);
    }
   if(pthread_create(&pthread,NULL,runNewClientThread,(void*)&new_socket)!=0){
			printf("failed to create a new thread\n");
			exit(EXIT_ERROR);
		}
    pthread_mutex_lock( &mutex );
    int curClientNum=clientNum;
    pthread_mutex_unlock( &mutex );
  }
  close(socket_fd);
  return EXIT_GOOD;
}


void* runNewClientThread(void* new_socket){
  pthread_mutex_lock( &mutex );
  int tmpClientNumber=clientNum++;
  pthread_mutex_unlock( &mutex );
  takeCareNewSocket(*(int*)new_socket, tmpClientNumber);
  return NULL;
}

int takeCareNewSocket(int curSockt,int curClientNum) {
  int msg_type=MSG_DUMP;
  while(MAX_CLIENT_NUM>curClientNum) {
    if(recv(curSockt,&msg_type,sizeof(msg_type),MSG_PEEK)<0){
      printf("recv failed\n");
      return EXIT_ERROR;
    }
    switch (msg_type){
      case MSG_NOTIFY:
        printf("Server - notify: receiving MSG_NOTIFY\n");
        getNewFileToSystem(curSockt);
        break;
      case MSG_DIRREQ:
        printf("Server - dirreq: Receiving MSG_DIRREQ\n");
        sendAllFilesOnSystem(curSockt);
        break;
      case MSG_SHUTDOWN:
        printf("Server - notify: Receiving MSG_SHUTDOWN\nBYE BYE\n");
        exit(EXIT_GOOD);
      case WAIT_TO_MSG:
        break;
      default:
        printf("\n******************************\n************************\nWE HAVE BIG THING TO FIX IN SERVER \n*********************\n*********************\n");
        return EXIT_ERROR;
    }
    msg_type = WAIT_TO_MSG;
  }
  return 1;
}



int startMainServer(struct sockaddr_in *myAddrass){
  int socket =  makeMeServerAndListen(myAddrass,P_SRV_PORT);
  printf("Server - server: opening socket on %s:%d\n", SERVER_IP_ADDRESS,P_SRV_PORT);
  return socket;
}

void getNewFileToSystem(int curSockt){

  msg_notify_t notify;
  msg_ack_t ack;
  file_ent_t curFile;

  if( recv(curSockt, &notify , sizeof(notify) , 0) < 0){
     printf("recv failed\n");
     exit(EXIT_ERROR);
  }

  ack.m_type = MSG_ACK;
  pthread_mutex_lock( &mutex );
  if(notify.m_port==0 && numOfFiles<MAX_FILES_NUM){
    ack.m_port=port++;
    printf("Server - notify: assigned port %d\n",ack.m_port);
  }
  else
    ack.m_port=notify.m_port;

  //add file to array
  if(numOfFiles>MAX_FILES_NUM)
    ack.m_port= -1;
  else{
    strcpy(allFiles[numOfFiles].fe_name,notify.m_name);
    allFiles[numOfFiles].fe_addr=notify.m_addr;
    allFiles[numOfFiles].fe_port=ack.m_port;
    numOfFiles++;
  }
pthread_mutex_unlock( &mutex );

  if( send(curSockt , &ack , sizeof(ack) , 0) < 0){
    printf("Send failed\n");
    exit(EXIT_ERROR);
  }
  printf("Server - notify: sending MSG_ACK\n");
 }

void sendAllFilesOnSystem(int curSockt){
  int i;
  msg_dirreq_t dirreq;
  msg_dirhdr_t dirhdr;
  msg_dirent_t dirent;

  if(recv(curSockt, &dirreq , sizeof(dirreq) , 0) < 0){
     printf("recv failed\n");
     exit(EXIT_ERROR);
  }
  dirhdr.m_type = MSG_DIRHDR;
  pthread_mutex_lock( &mutex );
  dirhdr.m_count = numOfFiles;
  pthread_mutex_unlock( &mutex );

  if( send(curSockt , &dirhdr , sizeof(dirhdr) , 0) < 0){
     printf("Send failed\n");
     exit(EXIT_ERROR);
  }
  printf("Server - dirreq: sending MSG_DIRHDR with count= %d\n",dirhdr.m_count);
  dirent.m_type = MSG_DIRENT;
  for(i=0;i<numOfFiles;i++){
    strcpy(dirent.m_name,allFiles[i].fe_name);
    dirent.m_addr = allFiles[i].fe_addr;
    dirent.m_port = allFiles[i].fe_port;
    if( send(curSockt , &dirent , sizeof(dirent) , 0) < 0){
      printf("Send failed\n");
      exit(EXIT_ERROR);
    }
  }

  printf("Server - dirreq: sent all MSG_DIRENT\n");

}
