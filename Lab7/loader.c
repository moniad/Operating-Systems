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

int max_pckgsCount_on_the_belt;

void parse_input(int argc, char **argv){
    if(argc == 0) die_errno("Not enough args passed to loader! Pass N and (maybe not) C!");
    pckg_weight = (int) strtol(argv[0], NULL, 10);
    if(argc == 3) {
        cycles = (int) strtol(argv[1], NULL, 10);
        max_pckgsCount_on_the_belt = (int) strtol(argv[2], NULL, 10);
    }
    printf("Yaya, loader!!! ");
    printf("Weight: %d, Cycles no: %d, max_pckgsCount: %d\n", pckg_weight, cycles, max_pckgsCount_on_the_belt);
}

void give_back_belt_op(){
    if(semop(sem_belt_operation_id, &sem_belt_operation_op_give, 1) < 0) die_errno("sem belt_op_give");
}

void get_sem_IDs(){
    if((sem_message_key = ftok(getenv("HOME"), PROJ_ID)) < 0) die_errno("ftok sem_msg");
    if((sem_belt_operation_key = ftok(getenv("HOME"), PROJ_ID+1)) < 0) die_errno("ftok sem_msg");
    if((sem_belt_weight_key = ftok(getenv("HOME"), PROJ_ID+2)) < 0) die_errno("ftok sem_msg");

    // get existing IDs -> nsems = 0
    if(((sem_message_id = semget(sem_message_key, 0, 0)) < 0) ||
       ((sem_belt_operation_id = semget(sem_belt_operation_key, 0, 0)) < 0) || 
       (sem_belt_weight_id = semget(sem_belt_weight_key, 0, 0)) < 0)
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
    printf("SHM_SIZE: %lu\n", SHM_SIZE(max_pckgsCount_on_the_belt));
    if((belt_id = shmget(belt_key, (size_t)SHM_SIZE(max_pckgsCount_on_the_belt), 0666)) < 0) die_errno("shmget()");
    printf("belt_id: %d, SHM_SIZE(K): %d\n", belt_id, (int) SHM_SIZE(max_pckgsCount_on_the_belt));
    if((belt = (package *) shmat(belt_id, NULL, 0)) < 0) die_errno("shmat()");
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    pid = getpid();
    get_sem_IDs();
    set_all_structs_sembuf();
    set_struct_sembuf(sem_belt_weight_op, 0, pckg_weight, IPC_NOWAIT);
    init_shm();
    while (cycles-- != 0){ // cause it can be equal to -1 if not specified
        flag = 1;
        printf("PID %d: Czekam na mozliwosc \"zgłoszenia się do truckera\"\n", pid);
        // printf("sem IDs: %d, %d, %d\n", sem_belt_operation_id, sem_belt_weight_id, sem_message_id);
        /* works
            package p = create_package();
            if(belt == NULL) printf("BUUUUUUUU\n");
            // if(belt[0] == NULL) printf("ahaaaaaaaa\n");
            belt[0] = p;
            print_package_details(p);
            printf("Pracownik PID = %d załadował paczkę o masie %d w chwili: %s\n", pid, pckg_weight, get_date_time());
        */

        union semun{
            int val;
        } arg;
        
        if((arg.val = semctl(sem_belt_operation_id, 0, GETVAL, arg)) < 0) die_errno("semctl belt_operation()");
        printf("WHY DONT YOU TAKE THIS SEMAPHORE??????????? arg.val: %d\n", arg.val);
        if(semop(sem_message_id, &sem_message_op_take, 1) < 0) die_errno("sem msg take");
        printf("NOM!");
        while(flag){
            printf("PID %d: Czekam na zwolnienie taśmy\n", pid);
            if(semop(sem_belt_operation_id, &sem_belt_operation_op_take, 1) < 0) die_errno("semop belt_op_take");
            if(semop(sem_belt_weight_id, &sem_belt_weight_op, 1)) { // OK
                // poloz paczke
                package p = create_package();
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
    if(shmdt(belt) < 0) die_errno("shmdt");
    printf("Loader: finished\n");
    return 0;
}
