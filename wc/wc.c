#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NOT_FILE ""
void calculateWC(int fileDes,int* lines,int* words,int* chars);
int isFileAndExist(const char *path);
void wcFromFile(char file[],int* lines,int* words,int* chars);

int main(int argc, char* argv[]){
  int lines=0,words=0,chars=0;
  if(argc == 1)
    wcFromFile(NOT_FILE,&lines,&words,&chars);
  else{
    for(int i=1;i<argc;i++){
      if(isFileAndExist(argv[i]))
        wcFromFile(argv[i],&lines,&words,&chars);
      }
    if(argc > 2)
      printf("%7d%7d%7d\ttotal\n",lines,words,chars);
  }
  return 1;
}

int isFileAndExist(const char *path){
  if( access( path, F_OK ) != -1 ) {
    struct stat path_stat;
    stat(path, &path_stat);

    if (S_ISDIR(path_stat.st_mode)){
      printf("wc: %s: Is a directory\n\t0\t0\t0  %s\n",path,path);
      return 0;
    }
    else{
        return 1;
    }
  }
  else {
        printf("wc: %s: No such file or directory\n", path);
        return 0;
  }
}



/* open calculate&save close save */

void calculateWC(int fileDes,int* lines,int* words,int* chars){
  char buff[512] = " ";
  int bufferSize = sizeof(buff);
  int readedBytes = bufferSize;

  int curLines=0,curWords=0,curChars=0;

  while(readedBytes > 0){ /* file is doesn't finish yet*/
    if( (readedBytes = read(fileDes,buff,bufferSize)) == -1){
      perror("problem with reading from file");
      exit(EXIT_FAILURE);
    }
    else{
      curChars += readedBytes;
      if(buff[0] == '\n')
        curLines++;
      for(int i=1;i<readedBytes;i++){
        if(isspace(buff[i]) && !isspace(buff[i-1]))
          curWords++;
        if(buff[i] == '\n')
          curLines++;
      }
    }
  }

  printf("%7d%7d%7d",curLines,curWords,curChars);
  (*lines)+=curLines;
  (*words)+=curWords;
  (*chars)+=curChars;
}

void wcFromFile(char file[],int* lines,int* words,int* chars){
  int fileDes;
  if( file == NOT_FILE)
    fileDes = STDIN_FILENO;
  else if( (fileDes = open(file,O_RDONLY)) == -1 ){
    perror("cant open file");
    exit(EXIT_FAILURE);
  }
  //The file is open
  calculateWC(fileDes,lines,words,chars);
  printf("  %s\n",file);

  if( (file != NOT_FILE)&&(close(fileDes) == -1) ){
    perror("problem with closing <FROM> file");
    exit(EXIT_FAILURE);
  }
}
