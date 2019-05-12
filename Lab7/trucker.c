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
#define _XOPEN_SOURCE // for strptime()
#include <time.h> // strptime()
#include "common.h"

// const char pathname[] = "/keypath";
pid_t child;
int capacity; // X
int max_pckgsCount_on_the_belt; // K
int max_pckgsWeight_on_the_belt; // M
int cur_pckg_no_in_truck;
struct sigaction act; // SIGINT handling
void SIGINThandler(int signum);

void parse_input(int argc, char **argv){
    if(argc != 4) die_errno("Give me:\n 1) X - the capacity of the truck\n 2) K - max no of packages on the belt\n 3) M - max total sum of packages' weight\n");
    capacity = (int) strtol(argv[1], NULL, 10);
    max_pckgsCount_on_the_belt = (int) strtol(argv[2], NULL, 10);
    max_pckgsWeight_on_the_belt = (int) strtol(argv[3], NULL, 10);
    // printf("%d, %d, %d\n", capacity, max_pckgsCount_on_the_belt, max_pckgsWeight_on_the_belt);
}

void create_and_init_semaphores(){
    // generate keys
    if((sem_message_key = ftok(getenv("HOME"), PROJ_ID)) < 0) die_errno("ftok sem_msg");
    if((sem_belt_operation_key = ftok(getenv("HOME"), PROJ_ID+1)) < 0) die_errno("ftok sem_msg");
    if((sem_belt_weight_key = ftok(getenv("HOME"), PROJ_ID+2)) < 0) die_errno("ftok sem_msg");

    // get IDs
    if((sem_message_id = semget(sem_message_key, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) die_errno("semget sem_mesg");
    if((sem_belt_operation_id = semget(sem_belt_operation_key, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) die_errno("semget sem_belt_operation");
    if((sem_belt_weight_id = semget(sem_belt_weight_key, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) < 0) die_errno("semget sem_belt_weight");

    set_all_structs_sembuf();

    // setting initial values
    union semun{ 
        int val;
    } arg;

    arg.val = max_pckgsWeight_on_the_belt;
    if(semctl(sem_belt_weight_id, 0, SETVAL, arg) < 0) die_errno("semctl belt_weight()");
    arg.val = 1;
    if(semctl(sem_belt_operation_id, 0, SETVAL, arg) < 0) die_errno("semctl belt_operation()");
    if(semctl(sem_message_id, 0, SETVAL, arg) < 0) die_errno("semctl message()");
}

void create_and_init_shm(){
    printf("creating shm\n");
    if((belt_key = ftok(getenv("HOME"), PROJ_ID-1)) < 0) die_errno("belt_key ftok");
    if((belt_id = shmget(belt_key, SHM_SIZE(max_pckgsCount_on_the_belt), IPC_CREAT | 0666 | IPC_EXCL)) < 0) die_errno("shmget()");
    if((belt = (package *) shmat(belt_id, NULL, 0)) < 0) die_errno("shmat()");
}

void rmv_sem_and_detach_shm(){
    // if(wait(NULL) < 0) die_errno("wait");
    printf("removing sem and shm\n");
    if(shmdt(belt) < 0) die_errno("shmdt");
    if(semctl(sem_message_id, 0, IPC_RMID) < 0) die_errno("removing sem_message");
    if(semctl(sem_belt_operation_id, 0, IPC_RMID) < 0) die_errno("removing sem_belt_op");
    if(semctl(sem_belt_weight_id, 0, IPC_RMID) < 0) die_errno("removing sem_belt_weight");
    if(shmctl(belt_id, IPC_RMID, NULL) < 0) die_errno("removing shared memory");
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

void move_belt_one_pos_forward(){
    for(int i = max_pckgsCount_on_the_belt-1; i > 0; i--)
        belt[i] = belt[i-1];
    belt[0].workers_pid = -1;
    belt[0].weight = -1;
    strcpy(belt[0].time_stamp, "null package");
}

time_t get_time(char date_time[]){
    struct tm tm_d_time;
    strptime(date_time, format, &tm_d_time); //convert string to date/time of type tm using format
   //get number of seconds from 1970 to given_date - equivalent to the type of m_time
    return mktime(&tm_d_time); //mktime() converts tm to time_t and the val of type time_t is returned
}

time_t count_time_diff(time_t t1, time_t t0){
    return difftime(t1, t0);
}

void move_one_pckg_to_truck(package p){
    cur_pckg_no_in_truck++;
    printf("Ładowanie paczki do ciezarowki: PID = %d, time_diff = %d s, paczek = 1, free = %d, taken = %d\n",
    p.workers_pid, (int) count_time_diff(get_time(get_date_time()), get_time(p.time_stamp)), capacity - cur_pckg_no_in_truck, cur_pckg_no_in_truck);
    sleep(2);
    if(cur_pckg_no_in_truck == capacity){
        printf("Brak miejsca. Odjazd i wyładowanie pełnej ciężarówki.\n");
        cur_pckg_no_in_truck = 0;
        sleep(2);
        printf("Podjechanie pustej ciężarówki\n");
    }
} 

int main(int argc, char **argv){
    parse_input(argc, argv);
    set_signal_handling();
    atexit(rmv_sem_and_detach_shm);
    create_and_init_semaphores();
    create_and_init_shm();
    cur_pckg_no_in_truck = 0;

    printf("%s\n", get_date_time());
    // testing
    package p;
    p.weight = 234;
    p.workers_pid = 500;
    
    strcpy(p.time_stamp, get_date_time());
    // sleep(15);
    // printf("ended waiting\n");
    belt[0] = p;
    printf("BELT[0]: %d, %d, %s\n", belt[0].workers_pid, belt[0].weight, belt[0].time_stamp);
    
    sleep(1);

    package p1;
    p1.weight = 123;
    p1.workers_pid = 555;
    strcpy(p1.time_stamp, get_date_time());
    belt[1] = p1;
    printf("BELT[1]: %d, %d, %s\n", belt[1].workers_pid, belt[1].weight, belt[1].time_stamp);
    
    sleep(2);
    
    capacity = 2;

    move_belt_one_pos_forward();
    printf("after moving: \n");
    for(int i = 0; i < 3; i++) print_package_details(belt[i]);

    move_one_pckg_to_truck(p);
    move_one_pckg_to_truck(p1);

    /*
    // TO NALEZY CIAGLE NA NOWO USTAWIAC!!!!!!!!!!!!!!!!
    // set_struct_sembuf(sem_belt_weight_op, 0, waga_zdjetej_paczki, 0);
    chyba z IPC_NOWAIT, wtedy pisze: "Czekam na załadowanie paczki"
    */

    // todo: count the difference between current time and time in the package
    // todo in SIGINThandler: zablokować semafor taśmy transportowej dla pracowników, załadować to, co pozostało na taśmie
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
    // sleep (2);
    printf("Done\n");
    return 0;
}
