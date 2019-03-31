// PO MODYFIKACJI TABLICY W INNEJ FUNKCJI DOSTAJEMY TABLICĘ Z 0-AMI W MAIN-IE
// CZYLI TO, CO W FUNKCJI, ZOSTAJE W FUNKCJI???
// z VFORK()-iem działa, ale tutaj nie :/

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
#include <libgen.h> // basename()
#include <linux/limits.h> // PATH_MAX
#include <dirent.h> // DIR

int tab[1025];

int doSth(int i){
    tab[i] = i;
    printf("TAB[%d]=%d\n", i, tab[i]);
    printf("TABLE in function:\n");
    for(int i=0; i<4; i++)
        printf("%d ", tab[i]);
    printf("\n");
    // while(1){
    for(int i=0; i<5; i++){    
        printf("i: %d Juhu!!\n", i);
        // sleep(2);
    }
    // return i;
    exit(0); //
}

int main(int argc, char **argv) {
    for(int i=0; i<1025; i++)
        tab[i] = -1;
    int children[4];
    for(int i=0; i<4; i++)
        children[i] = 0;
        
    for(int i=0; i<4; i++){
        pid_t child;
        if((child=vfork()) == 0){
            printf("%d, ", doSth(i));
        }
        else
            children[i] = child;
    }
    sleep(10);
    printf("CHILDREN:\n");
    for(int i=0; i<4; i++)
        printf("%d ", children[i]);
    for(int i=0; i<4; i++)
        kill(children[i], SIGINT);
    printf("\nTABLE in main:\n");
    for(int i=0; i<4; i++)
        printf("%d ", tab[i]);
}
