#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

long long get_time_us(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((long long)(ts.tv_sec) * 1000000000ll + (long long)(ts.tv_nsec))
            / 1000;
}

int main(int argc, char *argv[]) {
    if (argc != 2){
        printf("Please specify an input file!\n");
        return 0;
    }

    int pipefd[2]; // pipefd[0]: read endpoint, pipefd[1]: write endpoint
    pid_t cpid;
    char buf;
    long long start_us = get_time_us();

    if (pipe(pipefd) == -1) {
       printf("An error occured creating the pipe\n");
       return 0;
    }

    cpid = fork();

    if (cpid == 0) {          // Child process - Read from the pipe

	    // TODO: Read from Pipe and measure size of bytes read per line. Record how long it takes.
    }

      // TODO: Did we forget something?
      return 0;
    } else if (cpid > 0) {      // Parent writes file(argv[1]) to the pipe

	    //TODO: Read a file line by line and pass it to child process through pipe. Measure time it takes.
    }
	
      // TODO: Did we forget something
      wait(NULL);               // Wait for child to terminate
      return 0;
    } else {
      printf("An error occured forking a new process");
      return 0;
    }
}