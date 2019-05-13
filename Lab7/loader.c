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
pid_t pid;
int pckg_weight;
int cycles = -1; // C, max number of packages the worker will serve
int flag;
struct sembuf sem_message_op_take, sem_message_op_give;
struct sembuf sem_belt_operation_op_take, sem_belt_operation_op_give;
struct sembuf sem_belt_weight_op;
// int max_pckgsCount_on_the_belt;

void parse_input(int argc, char **argv){
    if(argc == 0) die_errno("Not enough args passed to loader! Pass N and (maybe not) C!");
    pckg_weight = (int) strtol(argv[0], NULL, 10);
    if(argc == 2) {
        cycles = (int) strtol(argv[1], NULL, 10);
        // max_pckgsCount_on_the_belt = (int) strtol(argv[2], NULL, 10);
    }
    printf("I'm a loader!!! ");
    printf("Weight: %d, Cycles no: %d,\n", pckg_weight, cycles); //, max_pckgsCount_on_the_belt);
}

void give_back_belt_op(){
    if(semop(sem_belt_operation_id, &sem_belt_operation_op_give, 1) < 0) die_errno("sem belt_op_give");
}

void get_sem_IDs(){
    if((sem_message_key = ftok(getenv("HOME"), PROJ_ID)) < 0) die_errno("ftok sem_msg");
    if((sem_belt_operation_key = ftok(getenv("HOME"), PROJ_ID+1)) < 0) die_errno("ftok sem_msg");
    if((sem_belt_weight_key = ftok(getenv("HOME"), PROJ_ID+2)) < 0) die_errno("ftok sem_msg");

    // get existing IDs -> nsems = 0
    if(((sem_message_id = semget(sem_message_key, 0, 0600)) < 0) ||
       ((sem_belt_operation_id = semget(sem_belt_operation_key, 0, 0600)) < 0) || 
       (sem_belt_weight_id = semget(sem_belt_weight_key, 0, 0600)) < 0)
       die_errno("Memory not initialized! Run trucker.c first!");
}

package create_package(){
    package p;
    p.workers_pid = pid;
    printf("package weight: %d\n", pckg_weight);
    p.weight = pckg_weight;
    printf(" vs p.weight : %d\n", p.weight);
    strcpy(p.time_stamp, get_date_time());
    return p;
}

void init_shm(){
    printf("init shm\n");
    if((belt_key = ftok(getenv("HOME"), PROJ_ID-1)) < 0) die_errno("belt_key ftok");
    printf("SHM_SIZE: %lu\n", SHM_SIZE(MAX_BELT_LENGTH));
    if((belt_id = shmget(belt_key, (size_t)SHM_SIZE(MAX_BELT_LENGTH), 0600)) < 0) die_errno("shmget()");
    printf("belt_id: %d, SHM_SIZE(K): %d\n", belt_id, (int) SHM_SIZE(MAX_BELT_LENGTH));
    if((belt = (package *) shmat(belt_id, NULL, 0)) < 0) die_errno("shmat()");
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    pid = getpid();
    get_sem_IDs();
    set_all_structs_sembuf(&sem_message_op_take, &sem_message_op_give,
        &sem_belt_operation_op_take, &sem_belt_operation_op_give);
    set_struct_sembuf(&sem_belt_weight_op, 0, pckg_weight, IPC_NOWAIT);
    init_shm();
    while (cycles-- != 0){ // cause it can be equal to -1 if not specified
        sleep(4);
        flag = 1;
        printf("PID %d: Czekam na mozliwosc \"zgłoszenia się do truckera\"\n", pid);
        int val;
        if((val = semctl(sem_message_id, 0, GETVAL, 0)) < 0) die_errno("semctl belt_operation()");
        printf("arg.val: %d\n", val);
        if(semop(sem_message_id, &sem_message_op_take, 1) < 0) die_errno("sem msg take");
        printf("HERE!!!!\n");
        while(flag){
            printf("PID %d: Czekam na zwolnienie taśmy\n", pid);
            if(semop(sem_belt_operation_id, &sem_belt_operation_op_take, 1) < 0) die_errno("semop belt_op_take");
            if(semop(sem_belt_weight_id, &sem_belt_weight_op, 1)) { // OK
                package p = create_package();
                // put package on the belt
                belt[0] = p;
                print_package_details(p);
                printf("Jestem pracownik o PID = %d i załadowałem paczkę o masie %d w chwili: %s\n", pid, pckg_weight, get_date_time());
                flag = 0;
                give_back_belt_op();
            }
            else if(errno == EAGAIN){
                printf("Errno, PID %d: Czekam na zwolnienie taśmy\n", pid);
                give_back_belt_op();
                continue;
            }
            else die_errno("semop belt_weight");
        }
        if(semop(sem_message_id, &sem_message_op_give, 1) < 0) die_errno("semop msg give");
        sleep(10);
    }

    if(shmdt(belt) < 0) die_errno("shmdt");
    printf("Loader: finished\n");
    return 0;
}
