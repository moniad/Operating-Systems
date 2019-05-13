#include <stdio.h> // perror()
#include <sys/ipc.h> // key_t
#include <sys/types.h> // semctl(), ftok()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include <signal.h>
#include <unistd.h> // sleep()
#include <errno.h>
#include <stdio.h> // fgets()
#include <wait.h>
#include <semaphore.h>
#include <sys/mman.h> // shared memory
#include <sys/stat.h>
#include <fcntl.h> // S_IWUSR itd.

#define PROJ_ID 7
#define MAX_BELT_LENGTH 100
#define SHM_SIZE MAX_BELT_LENGTH * sizeof(char)
// #define SHM_SIZE(K) (K*sizeof(struct package))
#define DATE_LENGTH 30

typedef struct package{
    pid_t workers_pid;
    int weight;
    char time_stamp[DATE_LENGTH];
} package;

const char shmname[] = "/shared_memory";
const char semname[] = "semafor1"; // "/semafor1"
sem_t *semid;
key_t semkey;
int shm_fd;
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

void create_and_init_shm(){
    printf("creating shm\n");
    if((shm_fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, 0600)) < 0) die_errno("shm_fd");
    printf("shm_fd: %d\n", shm_fd);
    // tell shm size:
    if(ftruncate(shm_fd, SHM_SIZE) < 0) die_errno("ftruncate()");
    // attach to process mem
    if((shmdata = (char*) mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
    MAP_SHARED, shm_fd, 0)) == (char *) -1) die_errno("mmap()");
    // close(shm_fd);
}

void rmv_sem_and_detach_shm(){
    printf("removing sem and shm\n");
    if(wait(NULL) < 0) die_errno("wait");
    if(sem_close(semid) < 0) die_errno("semclose()");
    if(sem_unlink(semname) < 0) die_errno("sem unlink()");

    // detach from proc mem
    if(munmap(shmdata, SHM_SIZE) < 0) die_errno("munmap()");
    // mark as 'to remove'
    if(shm_unlink(shmname) < 0) die_errno("shm_unlink()");
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    create_and_init_semaphore();
    create_and_init_shm();
    int valp;
    if(sem_getvalue(semid, &valp) < 0) die_errno("sem getvalue()");
    printf("Valp: %d\n", valp);

    child = fork();
    if(child != 0) atexit(rmv_sem_and_detach_shm);
    for(int i=0; i<3; i++){
        if(child != 0) {
                    if(sem_wait(semid) < 0) die_errno("sem take in parent"); // --
            if(!fgets(shmdata, SHM_SIZE, stdin)) die_errno("child, gets()");
            printf("parent - taken: i = %d, shmdata = %s\n", i, shmdata);
            strncpy(shmdata, "Parent\n", SHM_SIZE);
            if(sem_post(semid) < 0) die_errno("sem give in parent"); // ++
            sleep(1);
        }
        else {
            if(sem_wait(semid) < 0) die_errno("sem take in child");
            if(!fgets(shmdata, SHM_SIZE, stdin)) die_errno("child, gets()");
            printf("child - taken: i = %d, shmdata = %s\n", i, shmdata);
            strncpy(shmdata, "Child\n", SHM_SIZE);
            if(sem_post(semid) < 0) die_errno("sem give in child");
            sleep(1);
        }
    }
    printf("Done\n");
    return 0;
}
