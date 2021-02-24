/*
Dor Levi
203782735
*/
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/wait.h>

#define TRUE 1
#define BUFFER_SIZE 256
#define PATH_SIZE 256
#define MAX_WORD_IN_LINE 10
#define WORD_MAX_LENGTH 10
#define EXIT 0
#define CONTINUE 1
#define SON_STEEL_IN_WORK 2


int fixInputOutput(char** argumentsToSend,int numOfArgs);
int terminal();
void writePromt();
int runCmd();
int getLineToWords(char* line);
void freeArgumentsToSend();
int thereIsSign(char** argv,int* numOfArgs);
int readedBytes;
char buff[BUFFER_SIZE] = {'\0'};
char *curPath;
int retVal;
char arguments[MAX_WORD_IN_LINE][WORD_MAX_LENGTH];
char** argumentsToSend;
int numOfArgs;

int main (int argc,char* argv[]){
	curPath= malloc(sizeof(char) * 1024);
	argumentsToSend = malloc(sizeof(char*) * MAX_WORD_IN_LINE);
	printf("\n-------------------Dor shell ------------------\n");
  terminal();
	free(argumentsToSend);
  printf("\n----------------- End Dor shell -----------------\n");
  return 1;
}


int terminal(){
	while(TRUE){
		writePromt();
		if(runCmd() == EXIT)
			return EXIT;
	}
  return EXIT;
}

//chdir("/path/to/change/directory/to");
//curPath =
void writePromt(){
	curPath = getcwd(curPath, PATH_SIZE);
	write(1,"Dor shell : ",12);
	write(1,curPath,strlen(curPath));
	write(1," # ",2);
}

int runCmd(){
	int pid,i,status,Ampersent,outputFilePath=-1,inputFilePath=-1;
	char* filePathOutput=NULL,filePathInput;
	readedBytes = read(STDIN_FILENO, buff, BUFFER_SIZE);buff[readedBytes]='\0';
	numOfArgs = getLineToWords(buff);
	if(numOfArgs == 0){
		return CONTINUE;
	}
	Ampersent = thereIsSign(argumentsToSend,&numOfArgs);

	if(strcmp("cd",argumentsToSend[0]) == 0){
		if(numOfArgs > 2){
			printf("There is too many arguments\n");
		}
		else{
			if(numOfArgs == 1)
				retVal = chdir("/home");
			else
				retVal = chdir(argumentsToSend[1]);
			if(retVal != 0){
				printf("Dor bash: cd: %s: No such file or directory\n",argumentsToSend[1]);
			}
		}
		return CONTINUE;
	}

	if(strcmp("exit",argumentsToSend[0]) == 0){
		freeArgumentsToSend();
		return EXIT;
	}


	pid = fork();
	if(pid == 0){//son
			for (i = 0; i < numOfArgs; i++) {
				if( strcmp( argumentsToSend[i],"<") == 0){
					if(i+1 >numOfArgs){
						perror("syntax error near unexpected token 'newline'");
						return CONTINUE;
					}
					close(0);
					if( open( strdup(argumentsToSend[i+1]), O_RDONLY ) == -1 ){
						printf("%s isnt open\n", argumentsToSend[i+1]);
					return CONTINUE;
					}

				}
				if( strcmp( argumentsToSend[i],">") == 0){
					close(1);
					if( open( strdup(argumentsToSend[i+1]),  O_WRONLY | O_CREAT, 0666)  == -1 ){
						printf("%s isnt open\n", argumentsToSend[i+1]);
						return CONTINUE;
					}
					argumentsToSend[i]=NULL;
				}
			}

		execvp(strdup(argumentsToSend[0]),argumentsToSend);
		perror("execvp failed ");
		return CONTINUE;
	}
	else{//the father
		if( Ampersent != 1){
			waitpid(pid, &status, 0);
			freeArgumentsToSend();
		}
		return CONTINUE;
	}

}

int getLineToWords(char* line){
	int i=0,j=0,ctr=0;
	for(i=0;i<=strlen(line);i++)
	    {
	        // if space or NULL found, assign NULL into newString[ctr]
	        if(line[i]=='\0'||line[i]==' '||line[i]=='\n')
	        {
	             arguments[ctr][j]='\0';
							 if(strcmp(arguments[ctr],"") != 0){
							 argumentsToSend[ctr] = strdup(arguments[ctr]);
			 				 ctr++;  //for next word
							 j=0;    //for next word, init index to 0
						 }
					}
	        else
	            arguments[ctr][j++]=line[i];
	    }
			ctr++;
			argumentsToSend[ctr]=NULL;
			return ctr-1;
}

void freeArgumentsToSend(){
	int i;
	for(i=0;i<MAX_WORD_IN_LINE;i++){
		free(argumentsToSend[i]);
		argumentsToSend[i]=NULL;
	}
}

int thereIsSign(char **argv,int* numOfArgs){
	int n=*numOfArgs;
	if(	strcmp(argv[n-1],"&") == 0 )
		{
			argv[n-1]='\0';
			*numOfArgs=n-1;
			return 1;
		}
	else
	 	{
		 if (argv[n-1][strlen(argv[n-1])-1] == '&')
			{
			argv[n-1][strlen(argv[n-1])-1]='\0';
			return 1;
			}
		return 0;
	}
}
