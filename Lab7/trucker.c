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
#include <signal.h> // SIGINT handling
#include <time.h>
#include "common.h"

// const char pathname[] = "/keypath";
int shmid, semid;
key_t semkey, shmkey;
pid_t child;
int capacity; // X
int max_pckgsCount_on_the_belt; // K
int max_pckgsWeight_on_the_belt; // M
struct sigaction act; // SIGINT handling
void SIGINThandler(int signum);

void parse_input(int argc, char **argv){
    if(argc != 4) die_errno("Give me:\n 1) X - the capacity of the truck\n 2) K - max no of packages on the belt\n 3) M - max total sum of packages' weight\n");
    capacity = (int) strtol(argv[1], NULL, 10);
    max_pckgsCount_on_the_belt = (int) strtol(argv[2], NULL, 10);
    max_pckgsWeight_on_the_belt = (int) strtol(argv[3], NULL, 10);
    // printf("%d, %d, %d\n", capacity, max_pckgsCount_on_the_belt, max_pckgsWeight_on_the_belt);
}

void create_and_init_semaphore(){
    if((semkey = ftok(getenv("HOME"), PROJ_ID)) < 0) die_errno("ftok");
    if((semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) die_errno("semget");
    take.sem_num = give.sem_num = 0; // no_of_semaphore <=> index in the sem_base table
    take.sem_op = 1;
    give.sem_op = -1;
    take.sem_flg = give.sem_flg = 0;
}

void create_and_init_shm(){
    printf("creating sem\n");
    if((shmkey = ftok(getenv("HOME"), PROJ_ID+1)) < 0) die_errno("shmkey ftok");
    if((shmid = shmget(shmkey, SMH_SIZE, IPC_CREAT | 0666 | IPC_EXCL)) < 0) die_errno("shmget()");
    if((belt = shmat(shmid, NULL, 0)) == (char *) (-1)) die_errno("shmat()");
}

void rmv_sem_and_detach_shm(){
    // if(wait(NULL) < 0) die_errno("wait");
    printf("removing sem and shm\n");
    if(shmdt(belt) < 0) {
        // printf("errno: %d\n", errno);
        die_errno("shmdt");
    }
    if(semctl(semid, 1, IPC_RMID) < 0) die_errno("removing semaphore");
    if(shmctl(shmid, IPC_RMID, NULL) < 0) die_errno("removing shared memory");
    // system("ipcrm -a");
}

void set_signal_handling(){
    act.sa_handler = SIGINThandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) < 0) die_errno("sigaction");
    if(sigprocmask(SIG_SETMASK, &act.sa_mask, NULL) < 0) die_errno("sigprocmask");
}

void SIGINThandler(int signum){
    printf("Trucker.c: Received SIGINT. Quiting...");
    exit(0);
}
int main(int argc, char **argv){
    parse_input(argc, argv);
    set_signal_handling();
    atexit(rmv_sem_and_detach_shm);
    create_and_init_semaphore();
    create_and_init_shm();

    // i don't like the create_and_init_semaphore function()... wymusza wartości operacji na semaforze
    // check if the size of shared memory is correct
    // todo in SIGINThandler: zablokować semafor taśmy transportowej dla pracowników, załadować to, co pozostało na taśmie
    // W przypadku uruchomienia programu loader przed uruchomieniem truckera, powinien zostać wypisany odpowiedni komunikat 
    //      (obsłużony błąd spowodowany brakiem dostępu do nieutworzonej pamięci)
    // algorytm obsługi taśmy + warunek na masę paczek, pakowanie do ciężarówki
    // wypisywanie komunikatów u truckera i loaderów
    // in loaders_manager.c: the case when cycles is not given, doesn't work...

    // child = fork();
    // if(child != 0) 
    // // *belt = 0;
    // for(int i=0; i<3; i++){
    //     if(child != 0) {
    //         if(semop(semid, &take, 1) < 0) die_errno("semop take in parent");
    //         if(!fgets(belt, SMH_SIZE, stdin)) die_errno("child, gets()");
    //         printf("parent - taken: i = %d, belt = %s\n", i, belt);
    //           strncpy(belt, "Parent\n", SMH_SIZE);
    //       //   *belt = 10;
    //         if(semop(semid, &give, 1) < 0) die_errno("semop give in parent");
    //         sleep(1);
    //     }
    //     else {
    //         if(semop(semid, &take, 1) < 0) die_errno("semop take in child");
    //       //   *belt = 8;
    //         if(!fgets(belt, SMH_SIZE, stdin)) die_errno("child, gets()");
    //         printf("child - taken: i = %d, belt = %s\n", i, belt);
    //         strncpy(belt, "Child\n", SMH_SIZE);
    //         if(semop(semid, &give, 1) < 0) die_errno("semop give in child");
    //         sleep(1);
    //     }
    // }
    sleep (5);
    printf("Done\n");
    return 0;
}
