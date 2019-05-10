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

#define MAX_CYCLE_DIGITS 4

// const char pathname[] = "/keypath";
struct sembuf take, give;
pid_t *workers;
pid_t ppid;
int workersCount;
char **cycles;

void parse_input(int argc, char **argv){
    if(argc != 2) die_errno("Give me the number of workers\n");
    workersCount = (int) strtol(argv[1], NULL, 10);
    cycles = malloc(workersCount * sizeof(char *));
    for(int i = 0; i < workersCount; i++)
        cycles[i] = malloc(MAX_CYCLE_DIGITS * sizeof(char));
    for(int i = 0; i < workersCount; i++)
        scanf("%s", cycles[i]);
    // printf("%d\n", workersCount);
    // for(int i = 0; i < workersCount; i++)
    //     printf("%s ", cycles[i]);
}

void create_loader_processes(){
    for(int i=0; i<workersCount; i++){
        if((workers[i] = fork()) < 0) die_errno("fork()");
        if(workers[i] == 0){
            printf("cycles[i] : %s\n", cycles[i]);
            sleep(1);
            execl("loader.o", cycles[i], NULL);
        }
    }
}
int main(int argc, char **argv){
    parse_input(argc, argv);
    workers = malloc(workersCount * sizeof(pid_t));
    ppid = getpid();
    // *shmdata = 0;
    create_loader_processes();


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