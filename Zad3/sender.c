// sending SIGUSR1

#define _GNU_SOURCE //needed for FTW_PHYS and strptime() to work
#include <signal.h> // sigaction()
#include <stdio.h>
#include <string.h>
#include <stdlib.h> //calloc(), system(), exit()
#include <sys/stat.h> //stat()
#include <unistd.h> //getcwd(), sleep()
#include <time.h> //time_t, strptime(), ctime()
#include <fcntl.h> //open()
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

int counter = 0;
int no_of_signals;
const char *mode;
pid_t pid_of_catcher;

int check_input(int argc, char **argv);
void handleSIGUSR(int signum);
void send_signals(pid_t pid_of_catcher);

void handleSIGUSR(int signum){
    if(signum == SIGUSR1)
        counter++;
    else if(signum == SIGUSR2){
        printf("No. of received signals: %d\n", counter);
        printf("No. of sent signals: %d\n", no_of_signals);
        exit(0);
    }
    else printf("It mustn't happened!\n");
}

void send_signals(pid_t pid_of_catcher){
    // printf("%d, %d, %s\n", pid_of_catcher, no_of_signals, mode);
    if(strcmp(mode, "KILL") == 0){
        for(int i=0; i<no_of_signals; i++)
            kill(pid_of_catcher, SIGUSR1);
        kill(pid_of_catcher, SIGUSR2);
    }
    else if(strcmp(mode, "SIGACTION") == 0){
        // for(int i=0; i<no_of_signals; i++)
        //     kill(pid_of_catcher, SIGUSR1);
        // kill(pid_of_catcher, SIGUSR2);
    }
    else if(strcmp(mode, "SIGRT") == 0){
        // for(int i=0; i<no_of_signals; i++)
        //     kill(pid_of_catcher, SIGUSR1);
        // kill(pid_of_catcher, SIGUSR2);
    }
}

int parse_input(int argc, char **argv){
    if(argc != 4){
        printf("Give me 3 args: \n1) pid of catcher \n2) no. of signals to send \n3) mode\n");
        return -1;
    }
    // parse input
    pid_of_catcher = (pid_t) strtol(argv[1], NULL, 10);
    no_of_signals = (int) strtol(argv[2], NULL, 10);
    mode = argv[3];
    if(strcmp(mode, "KILL") != 0 && strcmp(mode, "SIGACTION") != 0 &&
        strcmp(mode, "SIGRT") != 0){
        printf("Incorrect mode. Try again!\n");
        return -1;
    }
    return 0;
}
int main(int argc, char **argv) {
    if(parse_input(argc, argv) < 0)
        return -1;

    send_signals(pid_of_catcher);

    printf("Check mode!!! \nSeems OK");

    return 0;
}

