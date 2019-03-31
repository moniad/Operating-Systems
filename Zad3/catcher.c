// catching SIGUSR1 and counting how many signals were sent

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
pid_t pid_of_sender;
const char *mode;

int check_input(int argc, char **argv);
int handleSIGUSR2(int signum);

void handleSIGUSR(int signum, siginfo_t *info, void *ucontext){
    if(signum == SIGUSR1){
        pid_of_sender = info->si_pid;
        counter++;
    }
    else if(signum == SIGUSR2){
        if(strcmp(mode, "KILL") == 0){
            for(int i=0; i<counter; i++)
                kill(pid_of_sender, SIGUSR1);
            kill(pid_of_sender, SIGUSR2);
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

        printf("No. of received signals: %d\n", counter);
        exit(0);
    }
    else printf("It mustn't happened!\n");
}

int parse_input(int argc, char **argv){
    if(argc != 2){
        printf("Give me 1 arg: \n1) mode\n");
        return -1;
    }
    mode = argv[1];
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

    // awaiting just two signals specified below
    struct sigaction act;
    act.sa_sigaction = handleSIGUSR;
    sigemptyset(&act.sa_mask);
    // sigfillset(&act.sa_mask); // all signals are blocked
    // sigprocmask(SIG_UNBLOCK, ) except 2
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGUSR2);
    act.sa_flags = SA_SIGINFO; // to know the pid of the sender
    sigaction(SIGUSR1, &act, NULL);

    printf("PID: %d\n", getpid());
    while(1){
        sleep(1);
    }
    printf("SENDER'S PID: %d\n", pid_of_sender);

    return 0;
}

