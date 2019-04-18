#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    pid_t child;
    if((child = fork()) == 0){
        printf("Child: exiting with 9\n");
        exit(10);
    }
    else {
        int status;
        wait(&status);
        if(WIFEXITED(status))
            printf("Parent: exit status of child: %d", WEXITSTATUS(status));
    }
    return 0;
}
