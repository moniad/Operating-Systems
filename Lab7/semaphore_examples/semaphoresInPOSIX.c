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
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h> // S_IWUSR itd.

#define PROJ_ID 7
#define SMH_SIZE 100

const char pathname[] = "/keypath";
const char semname[] = "semafor1"; // "/semafor1"
sem_t *semid;
// *shmid, 
key_t semkey, shmkey;
char *shmdata;
pid_t child;

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
    sem_unlink(semname);
    if((semid = sem_open(semname, O_CREAT | O_EXCL, 0600, 1)) < 0) die_errno("semopen()");
}

/*void create_and_init_shm(){
    printf("creating sem\n");
    if((shmkey = ftok(getenv("HOME"), PROJ_ID+1)) < 0) die_errno("shmkey ftok");
    if((shmid = shmget(shmkey, SMH_SIZE, IPC_CREAT | 0666 | IPC_EXCL)) < 0) die_errno("shmget()");
    if((shmdata = shmat(shmid, NULL, 0)) == (char *) (-1)) die_errno("shmat()");
}
*/
void rmv_sem_and_detach_shm(){
    if(wait(NULL) < 0) die_errno("wait");
    if(sem_close(semid) < 0) die_errno("semclose()");
    if(sem_unlink(semname) < 0) die_errno("sem unlink()");

    // printf("removing sem and shm\n");
    // if(shmdt(shmdata) < 0) {
    //     printf("errno: %d\n", errno);
    //     die_errno("shmdt");
    // }
    // if(semctl(semid, 1, IPC_RMID) < 0) die_errno("removing semaphore");
    // if(shmctl(shmid, IPC_RMID, NULL) < 0) die_errno("removing shared memory");
    // // system("ipcrm -a");
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    create_and_init_semaphore();
    // create_and_init_shm();

    int valp;
    if(sem_getvalue(semid, &valp) < 0) die_errno("sem getvalue()");
    printf("Valp: %d\n", valp);

    child = fork();
    if(child != 0) atexit(rmv_sem_and_detach_shm);
    // *shmdata = 0;
    for(int i=0; i<3; i++){
        if(child != 0) {
            // if(semop(semid, &take, 1) < 0) 
            if(sem_wait(semid) < 0) die_errno("sem take in parent"); // --

            // if(!fgets(shmdata, SMH_SIZE, stdin)) die_errno("child, gets()");
            printf("parent - taken: i = %d, shmdata = ...\n", i); //, shmdata);
        //       strncpy(shmdata, "Parent\n", SMH_SIZE);
        //   //   *shmdata = 10;
        //     if(semop(semid, &give, 1) < 0)
            if(sem_post(semid) < 0) die_errno("sem give in parent");
            sleep(1);
        }
        else {
            // if(semop(semid, &take, 1) < 0) 
            if(sem_wait(semid) < 0) die_errno("sem take in child");

          //   *shmdata = 8;
            // if(!fgets(shmdata, SMH_SIZE, stdin)) die_errno("child, gets()");
            printf("child - taken: i = %d, shmdata = ...\n", i); //, shmdata);
            // strncpy(shmdata, "Child\n", SMH_SIZE);
            // if(semop(semid, &give, 1) < 0)
            if(sem_post(semid) < 0) die_errno("sem give in child");
            sleep(1);
        }
    }
    printf("Done\n");
    return 0;
}
