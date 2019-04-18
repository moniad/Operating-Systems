// OK, works
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

int main(){
    pid_t child;
    int status, retval;
    if((child = fork()) < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
	if(child == 0) {
	// sleep(100000000);
	// while(1);
		exit(9);
    }
    else {
		printf("DOESN'T work properly :( Returns 0 all the time\n");
		if(waitpid(child, &status, WNOHANG) < 0){
			wait(&status);
			if(WIFEXITED(status))
				printf("Child PID: %d, EXIT STATUS: %d\n", child, WEXITSTATUS(status));
		}
		else {
			wait(&status);
			if(WIFEXITED(status))
				printf("Child PID: %d, EXIT STATUS: %d\n", child, WEXITSTATUS(status));
		}
		
    }
    exit(EXIT_SUCCESS);
}
