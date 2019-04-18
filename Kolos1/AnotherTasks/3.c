#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {
    int i, pid;

    if (argc != 2) {
        printf("Not a suitable number of program arguments");
        exit(2);
    } else {
        //*********************************************************
        //Uzupelnij petle w taki sposob aby stworzyc dokladnie argv[1] procesow potomnych, bedacych dziecmi
        //   tego samego procesu macierzystego.
        // Kazdy proces potomny powinien:
        // - "powiedziec ktorym jest dzieckiem",
        //-  jaki ma pid,
        //- kto jest jego rodzicem
        //******************************************************
        for(i = 0; i < atoi(argv[1]); i++){
            if((pid = fork()) == 0){
                printf("I'm child no. %d\n", i);
                printf("My pid is: %d\n", getpid());
                printf("My parent's pid is: %d\n", getppid());
                sleep(3);
                exit(0);
            }
        }  
    }
    return 0;
}