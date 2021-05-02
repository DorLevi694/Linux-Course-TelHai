#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "p2p.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define SEED 1
#define LEECH 2
#define SHUTDOWN 3

int checkIfHasFileToOtherClient(msg_dirent_t dirEnt);
int getFileSize(const char *file_name);
void seed(int argc,char* argv[],int serverSocket);
void leech(int argc,char* argv[],int serverSocket);
void shutdownAllSystem(int serverSocket);
int choseOpartion(char* str);
int checkParams(int argc,char op[]);
int connectToMainServer(struct sockaddr_in *server);
int connectSomeServer(struct sockaddr_in *server,in_addr_t addr,in_port_t port);
void makeMeServer();
int workOnNewConnection(int new_socket);
void moveFileFrom(char* fileReadFrom,int newSocket);
int moveFileTo(int peerSocket,int fileSize,char *fileName);

char* Files[10];
int numOfFiles=0;
char* myIP;
int port;

int main(int argc,char* argv[]){
  int serverSocket,mySocket,msg_type,opartion;
  struct sockaddr_in server,serverClient;
  myIP = strdup(SERVER_IP_ADDRESS); //change to func that give my ip
  opartion = checkParams(argc,argv[1]);
  serverSocket = connectToMainServer(&server);
  switch (opartion) {
    case SEED:
      seed(argc,argv,serverSocket);
      break;
    case LEECH:
      leech(argc,argv,serverSocket);
      break;
    case SHUTDOWN:
      shutdownAllSystem(serverSocket);
      return EXIT_GOOD;
      break;
  }
  makeMeServer();
  close(serverSocket);
  return EXIT_GOOD;
}

int choseOpartion(char* str){
  if (strcmp(str, "seed") == 0)
    return SEED;
  if (strcmp(str, "leech") == 0)
    return LEECH;
  if (strcmp(str, "shutdown") == 0)
    return SHUTDOWN;
  return MSG_ERROR;
}
int checkParams(int argc,char op[]){
  int opartion;
  if(argc < 2){
    printf("You need to write opartion\n");
    exit(EXIT_ERROR);
  }
  opartion = choseOpartion(op);
  if(opartion ==MSG_ERROR){
    printf("Wrong paramaters!\n");
    exit(EXIT_ERROR);
  }
  if(opartion != SHUTDOWN && argc == 2){
    printf("Your input should include files names after opartion!\n");
    exit(EXIT_ERROR);
  }
  return opartion;
}

int connectToMainServer(struct sockaddr_in *server){
  int conn=connectSomeServer(server,inet_addr(SERVER_IP_ADDRESS),P_SRV_PORT);
  if(conn == -1){
    printf("Can't connect main server\nBYE BYE\n");
    exit(EXIT_ERROR);
  }
  return conn;
}
int connectSomeServer(struct sockaddr_in *server,in_addr_t addr,in_port_t port){
  int serverSocket;
  server->sin_addr.s_addr = addr; //server connection
  server->sin_family = AF_INET;
  server->sin_port = htons( port );

  serverSocket = socket(AF_INET,SOCK_STREAM, 0);
  if(serverSocket == -1){
    printf("Could not create socket with server\n");
    exit(EXIT_ERROR);
  }

  if(connect(serverSocket, (struct sockaddr *)server, sizeof(*server)) <0){
    printf("connect error\n");
    return -1;
  }
  return serverSocket;
}

void makeMeServer(){
  if(numOfFiles == 0){
    printf("Client X - You have no files.. byebye\n");
    return;
}
  struct sockaddr_in myAddrass;
  int clientNum=0,new_socket,mySocket = makeMeServerAndListen(&myAddrass,port);
  printf("Peer   - start_server: starting peer server\n");
  printf("Peer   - start_server: bound socket to port %d\n",port);
  printf("Peer   - start_server: listning on socket\n");

  while(clientNum < MAX_CONNECTIONS){
    new_socket = accept(mySocket, (struct sockaddr*)NULL,(socklen_t*)NULL);
    if (new_socket == 0){
      perror("accept failed");
      exit(EXIT_ERROR);
    }
     printf("Peer   - start_server: accepted socket\n");
     if(workOnNewConnection(new_socket) == EXIT_GOOD){
      close(new_socket);
      return;
      }
      close(new_socket);
      clientNum++;

  }
}
int workOnNewConnection(int new_socket){
  msg_filereq_t fileReq;
  msg_filesrv_t fileSrv;
  int msg_type;
  if(recv(new_socket,&msg_type,sizeof(msg_type),MSG_PEEK)<0){
    printf("recv failed\n");
    exit(EXIT_ERROR);
  }
  if(msg_type == MSG_SHUTDOWN){
    recv(new_socket, &shutdown , sizeof(shutdown) , 0);
    printf("Client - get MSG_SHUTDOWN\n");
    return EXIT_GOOD;
  }
  if(msg_type != MSG_FILEREQ){
    printf("bad msg_type\nShould be 1 but we get %d\n",msg_type);
    exit(EXIT_ERROR);
  }
  if(recv(new_socket, &fileReq , sizeof(fileReq) , 0) < 0){
    printf("recv failed\n");
    exit(EXIT_ERROR);
  }

  //the msg is MSG_FILEREQ
  printf("Peer   - start_server: peeked at msg type:MSG_FILEREQ  -the file %s wanted\n",fileReq.m_name);

  fileSrv.m_type = MSG_FILESRV;
  fileSrv.m_file_size = getFileSize(fileReq.m_name);
  if(send(new_socket , &fileSrv , sizeof(fileSrv) , 0) < 0){
    printf("Send failed\n");
    close(new_socket);
    return CONTINUE;
  }

 if(fileSrv.m_file_size != -1 ){
    printf("Peer   - start_server: sending msg type:MSG_FILESRV\n");
    printf("Peer   - start_server: try to sending the file %s\n" ,fileReq.m_name);
    moveFileFrom(fileReq.m_name,new_socket);
  }
close(new_socket);
  return CONTINUE;
 }

int getFileSize(const char *file_name){
    struct stat st; /*declare stat variable*/
    /*get the size using stat()*/

    if(stat(file_name,&st)==0)
        return (st.st_size);
    else
        return -1;
}

void seed(int argc,char* argv[],int serverSocket){
  int i,msg_type;
  msg_notify_t notify;
  msg_ack_t ack;

  notify.m_type = MSG_NOTIFY;
  notify.m_addr = inet_addr(myIP);
  notify.m_port = 0;
  for(i=2;i<argc;i++){
    strcpy(notify.m_name,argv[i]);
    if(send(serverSocket , &notify , sizeof(notify) , 0) < 0){
      printf("Send failed\n");
      exit(EXIT_ERROR);
    }
    printf("Client - share: sending MSG_NOTIFY for %s @ %s:%d\n",notify.m_name ,myIP,notify.m_port);
    if(recv(serverSocket,&msg_type,sizeof(msg_type),MSG_PEEK)<0){
       printf("recv failed\n");
      exit(EXIT_ERROR);
    }
    if(msg_type != MSG_ACK){
      printf("bad msg_type\nShould be 1 but we get %d\n",msg_type);
      exit(EXIT_ERROR);
    }
    if(recv(serverSocket, &ack , sizeof(ack) , 0) < 0){
      printf("recv failed\n");
      exit(EXIT_ERROR);
    }
    printf("Client - notify: recving MSG_ACK for with port %d\n",ack.m_port);
    notify.m_port = ack.m_port;
    port = ack.m_port;
    if(port == -1)
      return;
    Files[numOfFiles] = strdup(notify.m_name);
    numOfFiles++;

  }
}
//return 1 if file move else -1
int checkIfHasFileToOtherClient(msg_dirent_t dirEnt){
  int peerSocket, mySocket,msg_type,fileMoved=-1;
  struct sockaddr_in peerAddr;
  msg_filereq_t fileReq;
  msg_filesrv_t fileSrv;

  peerSocket = connectSomeServer(&peerAddr,dirEnt.m_addr, dirEnt.m_port);
  if( peerSocket == -1){
    printf("peerSocket failed\n");
    return -1;
  }
  fileReq.m_type = MSG_FILEREQ;
  strcpy(fileReq.m_name,dirEnt.m_name);
  if(send(peerSocket , &fileReq , sizeof(fileReq) , 0) < 0){
    printf("Send failed\n");
    return -1;
  }
  if(recv(peerSocket , &msg_type,sizeof(msg_type),MSG_PEEK)<0){
    printf("error\nrecv failed\n");
   return -1;
  }
  if(msg_type != MSG_FILESRV){
    printf("bad msg_type\nShould be 7(MSG_FILESRV) but we get %d\n",msg_type);
    exit(EXIT_ERROR);
  }
  if(recv(peerSocket, &fileSrv , sizeof(fileSrv) , 0) < 0){
    printf("error\nrecv failed\n");
    return -1;
  }
  if(fileSrv.m_file_size != -1){
    fileMoved=moveFileTo(peerSocket,fileSrv.m_file_size,fileReq.m_name);
    if(fileMoved == 1)
      printf("Successed(is size is:%d)\n",fileSrv.m_file_size);
    else
       printf("Failed To move\n");
   }
   else
      printf("Failed - The file dosent exist\n");


  return fileMoved;
}

int moveFileTo(int peerSocket,int fileSize,char *fileName){
  int fileWrite;
  char buff[P_BUFF_SIZE] = " ";
  int bufferSize = sizeof(buff);
  int readedBytes = bufferSize;
  int AllReadedBytes=0;

  if( (fileWrite = open(fileName,O_WRONLY|O_CREAT,0644)) == -1 )
    return -1;

  while(readedBytes != -1 && AllReadedBytes<fileSize){
    if((readedBytes = recv(peerSocket, buff , P_BUFF_SIZE , 0)) < 0)
      return -1;
    AllReadedBytes+=readedBytes;
    if( write(fileWrite,buff,readedBytes) == -1)
      return -1;
  }
  if( close(fileWrite) == -1){
    return -1;
  }
  return 1;
}
void moveFileFrom(char* fileReadFrom,int new_socket){
    int fileRead,  fileWrite,fileSize=0;
    char buff[P_BUFF_SIZE];;
    int bufferSize = sizeof(buff);
    int readedBytes = bufferSize;

    if( (fileRead = open(fileReadFrom,O_RDONLY)) == -1 ){
      return;
    }


    while(readedBytes == bufferSize){
      if( (readedBytes = read(fileRead,buff,bufferSize)) == -1){
        printf("Peer   - problem with reading from file: %s\n",fileReadFrom);
        close(fileRead);
        return;
        }
      fileSize+=readedBytes;
      if(send(new_socket , buff , readedBytes , 0) < 0){
        printf("Peer   - Send failed\n");
        close(fileRead);
        return;
      }
    }

     if( close(fileRead) == -1){
       printf("Peer    - problem with closing %s file\n",fileReadFrom);
       return;
     }
     printf("Peer    - start_server: The file %s moved(is size:%d)\n", fileReadFrom,fileSize);
  }


void shutdownAllSystem(int serverSocket){
  int i,j,msg_type,new_socket;
  msg_dirreq_t dirReq;
  msg_dirhdr_t dirHdr;
  msg_dirent_t dirEntArr[MAX_FILES_NUM];
  struct sockaddr_in server;
  msg_shutdown_t shutdown;
  shutdown.m_type = MSG_SHUTDOWN;
  dirReq.m_type = MSG_DIRREQ;

  if( send(serverSocket , &dirReq , sizeof(dirReq) , 0) < 0){
    printf("Send failed\n");
    exit(EXIT_ERROR);
  }
  if( recv(serverSocket,&msg_type,sizeof(msg_type), MSG_PEEK) <0){
    printf("recv failed\n");
    exit(EXIT_ERROR);
  }
  if(msg_type != MSG_DIRHDR){
    printf("bad msg_type\nShould be 4(MSG_DIRHDR) but we get %d\n",msg_type);
    exit(EXIT_ERROR);
  }
  if(recv(serverSocket, &dirHdr , sizeof(dirHdr) , 0) < 0){
    printf("recv failed\n");
    exit(EXIT_ERROR);
  }
  printf("client - sending MSG_DIRHDR=%d\n",dirHdr.m_count);

 for(i=0;i<dirHdr.m_count;i++){
    if(recv(serverSocket,&msg_type,sizeof(msg_type),MSG_PEEK)<0){
      printf("recv failed\n");
      exit(EXIT_ERROR);
    }
    if(msg_type != MSG_DIRENT){
      printf("bad msg_type\nShould be 5(MSG_DIRENT) but we get %d\n",msg_type);
      exit(EXIT_ERROR);
    }
    if(recv(serverSocket, &(dirEntArr[i]), sizeof(dirEntArr[0]), 0)<0){
      printf("recv failed\n");
      exit(EXIT_ERROR);
    }
 }
 for(i=0;i<dirHdr.m_count;i++){
  new_socket=connectSomeServer(&server,dirEntArr[i].m_addr,dirEntArr[i].m_port);
  if(new_socket!=-1){
    send(new_socket , &shutdown , sizeof(shutdown) , 0);
    close(new_socket);
  }
 }
 send(serverSocket , &shutdown , sizeof(shutdown) , 0);
 close(serverSocket);
 return;
};



void leech(int argc,char* argv[],int serverSocket){
  int i,j,findGoodFile,msg_type;

  msg_dirreq_t dirReq; dirReq.m_type = MSG_DIRREQ;
  msg_dirhdr_t dirHdr;dirHdr.m_type = MSG_DIRHDR;
  msg_filereq_t fileReq;fileReq.m_type = MSG_FILEREQ;
  msg_filesrv_t fileSrv;fileSrv.m_type = MSG_FILESRV;

  msg_dirent_t dirEntArr[MAX_FILES_NUM];
  char* tempFiles[12];


  if( send(serverSocket , &dirReq , sizeof(dirReq) , 0) < 0){
    printf("Send failed\n");
    exit(EXIT_ERROR);
  }
  printf("Client - get_list: sending MSG_DIRREQ\n");
  if( recv(serverSocket,&msg_type,sizeof(msg_type),MSG_PEEK)<0){
    printf("recv failed\n");
    exit(EXIT_ERROR);
  }
  if(msg_type != MSG_DIRHDR){
    printf("bad msg_type\nShould be 4(MSG_DIRHDR) but we get %d\n",msg_type);
    exit(EXIT_ERROR);
  }
  if(recv(serverSocket, &dirHdr , sizeof(dirHdr) , 0) < 0){
    printf("recv failed\n");
    exit(EXIT_ERROR);
  }
  printf("Client - get_list: receiving MSG_DIRHDR with %d items \n",dirHdr.m_count);

  for(i=0;i<dirHdr.m_count;i++){
    if(recv(serverSocket,&msg_type,sizeof(msg_type),MSG_PEEK)<0){
      printf("recv failed\n");
      exit(EXIT_ERROR);
    }
    if(msg_type != MSG_DIRENT){
      printf("bad msg_type\nShould be 5(MSG_DIRENT) but we get %d\n",msg_type);
      exit(EXIT_ERROR);
    }
    if(recv(serverSocket, &(dirEntArr[i]), sizeof(dirEntArr[0]), 0)<0){
      printf("recv failed\n");
      exit(EXIT_ERROR);
    }
    printf("Client - get_list: received MSG_DIRENT for file %s served by peer at %d:%d\n",dirEntArr[i].m_name,dirEntArr[i].m_addr,dirEntArr[i].m_port);
  }
  for(i=2;i<argc;i++){
  //move on the files the client want
      strcpy(fileReq.m_name,argv[i]);
      for(j=0;j<dirHdr.m_count;j++){

          findGoodFile = 0;
          if( strcmp(fileReq.m_name,dirEntArr[j].m_name) ==0 ){
            printf("Peer   - Found file %s- try move..result: ",fileReq.m_name);
            findGoodFile = checkIfHasFileToOtherClient(dirEntArr[j]);
            if(findGoodFile == 1){
              tempFiles[numOfFiles+2] = strdup(fileReq.m_name);
              numOfFiles++;
              break;
            }
            else
              printf("Peer   - try to look for other peer for the file..\n");
          }
      }
      if(findGoodFile != 1)
          printf("Peer   - File %s is not moved\n",fileReq.m_name);
  }

 seed(numOfFiles+2, tempFiles,serverSocket);
}
