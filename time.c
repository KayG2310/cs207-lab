#include <stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

int main(int argc, char* argv[]){
    struct timeval start_time, end_time;
    if(argc<2){
        printf("Fewer arguments than expected");
        return 1;
    }
    char command[200];
    strcpy(command, argv[1]);
    pid_t pid;
    pid = fork();

    if(pid<0){
        printf("Fork not successful\n");
        return 2;
    }
    else if(pid == 0){
        // inside child process

    }

}

