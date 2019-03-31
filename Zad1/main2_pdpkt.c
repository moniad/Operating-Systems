// WORKS!
// SIGSTOP can't be ignored while SIGTSTP might be.
#include <stdio.h>
#include <signal.h> // sigaction()
#include <unistd.h> // sleep()
#include <stdlib.h> // exit()
#include <time.h>
#include <sys/wait.h>

int run = 0;
pid_t child;

void handleSIGTSTP(int signum){
    int status;
    // printf("GET PID IN FUNCTION: %d\n", getpid());
    // printf("CHILD IN FUNCTION: %d\n", child);
    // if(waitpid(child, &status, WNOHANG) == 0){ // 0 is returned
    if(run == 1){
        run = 0;
        // printf("No terminated children. Continuing with printing date!\n");
        child = fork();
        if(child == 0) execl("./data.sh", "data.sh", NULL);
    }
    else{
        run++;
        // printf("MY CHILD HAS ENDED ITS LIFE :(\n");
        kill(child, SIGINT);
    }
}

void handleSIGINT(int signum){
    printf("\nOdebrano sygnał SIGINT\n");
    kill(getpid(), SIGINT);
    exit(0);
}

int main(void) {
    time_t now;
    struct tm *time_info;

    signal(SIGTSTP, handleSIGTSTP);
    signal(SIGINT, handleSIGINT);

    // printf("GET PID BEFORE FUNCTION: %d\n", getpid());
    child = fork();
    while(1){
        if(child == 0) 
            execl("./date.sh", "date.sh", NULL);
        // printf("Still working\n");
        sleep(2);
    }
}