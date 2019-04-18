// doesn't work properly :<
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sighandler(int sig, siginfo_t *info, void *ucontext){
    printf("ARGUMENT: %d", info->si_value.sival_int);
    exit(0);
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Give me the following params: 1. value to be passed to child process, ");
        printf("2. SIGNAL\n");
        return 1;
    }
       
  // => zablokuj wszystkie sygnaly za wyjatkiem SIGUSR1
        //zdefiniuj obsluge SIGUSR1 w taki sposob zeby proces potomny wydrukowal
        //na konsole przekazana przez rodzica wraz z sygnalem SIGUSR1 wartosc

            //wyslij do procesu potomnego sygnal przekazany jako argv[2]
        //wraz z wartoscia przekazana jako argv[1]

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);

    struct sigaction act;
    act.sa_sigaction = &sighandler;
    act.sa_mask = set;
    act.sa_flags = SA_SIGINFO;

    sigprocmask(SIG_SETMASK, &set, NULL);
    sigaction(SIGUSR1, &act, NULL);

    // printf("set\n");
    pid_t child;
    if((child = fork()) == 0){
        // printf("child PID %d\n", getpid());
        pause();
    }
    else {
        union sigval value;
        value.sival_int = (int) strtol(argv[1], NULL, 10);
        // printf("%d\n", value.sival_int);
        // printf("child pid: %d\n", child);
        printf("SIGNAL: %d", (int) strtol(argv[2], NULL, 10));
        // sigqueue(child, , value);
        // printf("sent\n");
    }
    exit(0);
}

// // works fine! problem with converting signal string to number
// #include <stdio.h>
// #include <stdlib.h>
// #include <signal.h>
// #include <unistd.h>

// void sighandler(int sig, siginfo_t *info, void *ucontext){
//     printf("ARGUMENT: %d", info->si_value.sival_int);
//     exit(0);
// }

// int main(int argc, char *argv[]) {

//     sigset_t set;
//     sigfillset(&set);
//     sigdelset(&set, SIGUSR1);

//     struct sigaction act;
//     act.sa_sigaction = &sighandler;
//     act.sa_mask = set;
//     act.sa_flags = SA_SIGINFO;

//     sigprocmask(SIG_SETMASK, &set, NULL);
//     sigaction(SIGUSR1, &act, NULL);

//     // printf("set\n");
//     pid_t child;
//     if((child = fork()) == 0){
//         // printf("child PID %d\n", getpid());
//         pause();
//     }
//     else {
//         union sigval value;
//         value.sival_int = 56;
//         // printf("%d\n", value.sival_int);
//         // printf("child pid: %d\n", child);
//         sigqueue(child, SIGUSR2, value);
//         sigqueue(child, SIGUSR1, value);
//         // printf("sent\n");
//     }
//     exit(0);
// }