#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h> // perror()
#include <stdlib.h> // exit()
#include <sys/sem.h> // struct sembuf
#include <time.h>

#define PROJ_ID 7
#define SHM_SIZE(K) (K*sizeof(struct package))
#define DATE_LENGTH 30

typedef struct package{
    pid_t workers_pid;
    int weight;
    char time_stamp[DATE_LENGTH];
} package;

key_t belt_key, sem_message_key, sem_belt_operation_key, sem_belt_weight_key;
int belt_id, sem_message_id, sem_belt_operation_id, sem_belt_weight_id;
struct sembuf sem_message_op_take, sem_message_op_give;
struct sembuf sem_belt_operation_op_take, sem_belt_operation_op_give;
struct sembuf sem_belt_weight_op;
package *belt; // shared memory
char datetime[DATE_LENGTH];

// mam K miejsc w belt[]. w belt[i] trzymam paczkę i przesuwam wskaźnikiem (tam, gdzie wskaźnik,
// odbieram paczkę) od końca 
// do początku i wracam na koniec z powrotem (pętla). <- symulacja ruchu przesuwającego taśmę
// jest 1 semafor do operacji na taśmie, tj. wsadzania przez loadera na pierwsze miejsce na taśmie LUB
// ściągania przez truckera z ostatniego

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

void set_struct_sembuf(struct sembuf str, int num, int op, int flag){
    str.sem_num = num;
    str.sem_op = op;
    str.sem_flg = flag;
}

void set_all_structs_sembuf(){
    set_struct_sembuf(sem_message_op_take, 0, -1, 0);
    set_struct_sembuf(sem_message_op_give, 0, 1, 0);
    set_struct_sembuf(sem_belt_operation_op_take, 0, -1, 0);
    set_struct_sembuf(sem_belt_operation_op_give, 0, 1, 0);
}

char *get_date_time(){ // datetime is statically allocated, but it's not a problem
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if(strftime(datetime, DATE_LENGTH, "%d-%m-%Y %H:%M:%S", &tm) == 0)
        die_errno("strftime");
    return datetime;
}

#endif