#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#define MAXLINE 80 /* The maximum length of a command line */ 
#define MAXARGS 40 /* The maximum # of tokens in command line */ 
void spool(char * args[], int length);
/* NOTE: replace FirstName, LastName, and L3SID */ 
int main(void) 
{ 
  char cmdline[MAXLINE]; 
  char *args[MAXARGS]; 
  char * token;
  int ret;
  int status;
  //... 
  printf("CS149 Spring 2023 Shellpipe from Dawson Williams\n"); 
  while (1) { 
    int shouldWait = 1; //default is for parent procs to wait for child procs
    int hasSpooled = 0; //will check if using the | operator (handled in spool() function)
    printf("Dawson-071>"); 
    fflush(stdout); 
    // (1) shell - read user input (stdin) to cmdline 
    fgets(cmdline, MAXLINE, stdin);
    // (2) shell - parse tokens from cmdline to args[] 
    token = strtok(cmdline, " \n"); //get first token from cmdline
    //if an empty string was entered or whitespace, create a new line and prompt
    if(token == NULL){
        continue;
    }
    int length = 0;
    while(token != NULL){ //if it isnt the end of the string, add it to args
        args[length] = token;
        length++;
        token = strtok(NULL, " \n"); //grab the next token from where it left off and remove \n from fgets()
    } 
    //check if you parent process should wait on child (default is to wait)
    if(strcmp(args[length - 1], "&") == 0){
        shouldWait = 0;
        length--; //decrement length to write over & with null when making cmd
    }
    //check for pipe operator
    for(int i = 0; i < length; i++){
        if(strcmp(args[i], "|") == 0){
            spool(args, length);
            hasSpooled = 1;
            break;
        }
    }
    //if the | operator was used, logic handled in spool function, return to top
    if(hasSpooled){
        continue;
    }

    //add null at the end of args
    args[length] = NULL;
    length++;

    //create path to system programs path
    char path[20] = "/bin/";
    args[0] = strcat(path, args[0]);

    // (3) shell - if user enters exit, terminate the shell (parent) 
    if(strcmp(cmdline, "exit") == 0){ //if exit\n was entered, return from the program entirely
        return 0;
    }

    // (4) shell - fork a child process using fork()
    ret = fork();
    if(ret < 0){//error
        printf("Fork error\n");
    } 

    // (5) child - call execvp() 
    else if(ret == 0){ //child process
        execvp(args[0], args);
        printf("Execute error for command:%s \n", args[0]);
        exit(1);
    }

    // (6) shell – if no & at the end of cmdline, call wait() to wait for the termination of the child, else do nothing 
    else{//parent process
    if(shouldWait){
        wait(&status);
    }

    }

    // (7) shell - go back to (1) 
    
  } 
  return 0; 
}

void spool(char * args[], int length){
    //init argument 1 and argument 2 with half the size of maxsize
    int pipeFound = 0;
    char * arg1[MAXARGS / 2];
    int arg1Length = 0;
    char * arg2[MAXARGS / 2];
    int arg2Length = 0;
    int j = 0; //used to index at the start of argument 2
    int fd[2];//file descriptors
    int ret1; // return for beginning shell -> cmd2 fork
    int ret2; // return for secondary cmd2 -> cmd1 fork
    int status; // wait status
    for(int i = 0; i < length; i++){
        //check for the pipe operator. If found, will start arg2
        if(strcmp(args[i], "|") == 0){
            pipeFound = 1;
            continue;
        }
        //if the pipe is yet to be found, still building arg1
        if(!pipeFound){
            arg1[i] = args[i];
            arg1Length++;
        }
        //build arg2 with j
        else{
            arg2[j] = args[i];
            j++;
            arg2Length++;
        }
    }

    //add null at the end of args
    arg1[arg1Length] = NULL;
    arg1Length++;

    arg2[arg2Length] = NULL;
    arg2Length++;

    //create path to system programs path
    char pathBuffer1[20] = "/bin/";
    char pathBuffer2[20] = "/bin/";
    arg1[0] = strcat(pathBuffer1, arg1[0]);
    arg2[0] = strcat(pathBuffer2, arg2[0]);
    
    
    //parent -> arg2(child) -> arg1(grandchild) processes (farthest child in tree has highest priority to finish if parents wait)
    ret1 = fork(); //initial fork to arg2 child and shell parent
    if(ret1 < 0){ //error handle
        printf("Fork error\n");
        exit(1);
    }


    else if(ret1 == 0){ //child process (arg2) of shell (parent)
        pipe(fd);//create message pipe for processes
        ret2 = fork(); //create grandchild process (cmd1) and child process (cmd2)
        if(ret2 < 0){ //error handle
            printf("Fork error\n");
            exit(1);
        }

        else if(ret2 == 0){ //grandchild process of shell, child process of cmd2, process cmd1
            close(fd[0]);//close write end of the pipe
            dup2(fd[1], STDOUT_FILENO);//redirect stdout to write end of the pipe
            execv(arg1[0], arg1);
            printf("Exec error for command: %s\n", arg2[0]);
            exit(1);
        }
        else{ //cmd2: child of the shell, parent of cmd1
        wait(&status); //await cmd1 to finish, then execute cmd2
        close(fd[1]);//close read end of the pipe
        dup2(fd[0], STDIN_FILENO);//redirect stdin to read end of the pipe
        execv(arg2[0], arg2);
        printf("Exec error for command: %s\n", arg1[0]);
        exit(1);
        }
    } 
    else{
    //await arg2 to finish before shell will resume
    wait(&status);  
    return;
    }
}