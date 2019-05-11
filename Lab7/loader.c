#include <stdio.h> // perror()
#include <sys/types.h> // semctl(), ftok()
#include <sys/ipc.h> // semctl(), ftok()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include <signal.h>
#include <unistd.h> // sleep()
#include <errno.h>
#include <stdio.h> // fgets()
#include <wait.h>
#include <sys/sem.h> // semctl()
#include <sys/shm.h> // shared memory ops
#include <sys/stat.h> // S_IWUSR itd.
#include <fcntl.h> // S_IWUSR
#include "common.h"

// const char pathname[] = "/keypath";
struct sembuf take, give;
pid_t pid;
int pckg_weight;
int cycles = -1; // C, max packages number per worker!

void parse_input(int argc, char **argv){
    if(argc == 0) die_errno("Not enough args passed to loader! Pass N and (maybe not) C!");
    pckg_weight = (int) strtol(argv[0], NULL, 10);
    if(argc == 2) cycles = (int) strtol(argv[1], NULL, 10);
    printf("Yaya, loader!!! ");
    printf("Weight: %d, Cycles no: %d\n", pckg_weight, cycles);
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    pid = getpid();
   
    // while cycles != 0 <- it can be -1 if no conditions
    /*
        if(child != 0) {
            if(semop(semid, &take, 1) < 0) die_errno("semop take in parent");
            if(!fgets(shmdata, SMH_SIZE, stdin)) die_errno("child, gets()");
            printf("parent - taken: i = %d, shmdata = %s\n", i, shmdata);
              strncpy(shmdata, "Parent\n", SMH_SIZE);
          //   *shmdata = 10;
            if(semop(semid, &give, 1) < 0) die_errno("semop give in parent");
            sleep(1);
        }
        else {
            if(semop(semid, &take, 1) < 0) die_errno("semop take in child");
          //   *shmdata = 8;
            if(!fgets(shmdata, SMH_SIZE, stdin)) die_errno("child, gets()");
            printf("child - taken: i = %d, shmdata = %s\n", i, shmdata);
            strncpy(shmdata, "Child\n", SMH_SIZE);
            if(semop(semid, &give, 1) < 0) die_errno("semop give in child");
            sleep(1);
        }
    }
    */
    // i++;
    // printf("Done\n");
    // for(int j = 0; j < workersCount; j++)
    //     if(getpid() == workers[j]) break;
    //     else{
    //         if(wait(NULL) < 0) die_errno("wait()");
    //         printf("%d\n", i);
    //     }
    return 0;
}
