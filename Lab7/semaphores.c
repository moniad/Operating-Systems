#include <stdio.h> // perror()
#include <sys/types.h> // semctl(), ftok()
#include <sys/ipc.h> // semctl(), ftok()
#include <sys/msg.h> // msgget()
#include <sys/stat.h> // chmod()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include <signal.h>
#include <unistd.h> // sleep()
#include <ctype.h> // isdigit()
#include <errno.h>

#include <sys/sem.h> // semctl()
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>

#define PROJ_ID 7

// const char pathname[] = "/key_path";
struct sembuf take, give;
int shmid, semid;
key_t semkey;

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

void parse_input(int argc, char **argv){
    // if(argc < 2)
    //     strcpy(jobs_file_name, def_file_name);
    // else
    //     strcpy(jobs_file_name, argv[1]);
}

void create_and_init_semaphore(){
    if((semkey = ftok(getenv("HOME"), PROJ_ID)) < 0) die_errno("ftok");
    if((semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) die_errno("semget");
    
    take.sem_num = give.sem_num = 0; //initialization
    take.sem_op = 1;
    give.sem_op = -1;
    take.sem_flg = give.sem_num = 0;
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    create_and_init_semaphore();
    pid_t child = fork();
    for(int i=0; i<3; i++)
        if(child == 0) {
            if(semop(semid, &take, 1) < 0) die_errno("semop take in child");
            printf("Child\n");
            if(semop(semid, &give, 1) < 0) die_errno("semop give in child");
            sleep(1);
        }
        else {
            if(semop(semid, &take, 1) < 0) die_errno("semop take in parent");
            printf("Parent\n");
            if(semop(semid, &give, 1) < 0) die_errno("semop give in parent");
            sleep(1);
        }
    return 0;
}