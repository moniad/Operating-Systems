#include <stdio.h>
#include <signal.h> // sigaction()
#include <unistd.h> // sleep()
#include <stdlib.h> // exit()
#include <time.h>

int run = 0;

void handleFirstSIGTSTP(int signum){
    if(run == 0){
        printf("\nOczekuję na CTRL+Z - kontynuacja albo CTRL+C - zakończenie programu\n");
        run++;
    }
    else run = 0;
}

void handleSIGINT(int signum){
    printf("\nOdebrano sygnał SIGINT\n");
    exit(0);
}

int main(void) {
    time_t now;
    struct tm *time_info;

    struct sigaction act;
    act.sa_handler = handleFirstSIGTSTP;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    // tylko raz ??
    sigaction(SIGTSTP, &act, NULL);
    signal(SIGINT, handleSIGINT);

    while(1){
        if (run == 0) {
            time(&now);
            time_info = localtime (&now);
            printf ("Current local time and date: %s\n", asctime(time_info));
        }
        sleep(1);
    }
}