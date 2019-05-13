
// DOKUMENTACJA:
// mam K miejsc w belt[]. Do belt[0] dokładam paczkę i odbieram paczkę spod ostatniego indeksu 
// są 3 semafory:
// - jeden - binarny - tylko dla loaderów do wkładania paczek na taśmę
// - drugi - binarny - dla truckera i loaderów, aby w jednej chwili była wykonywana
//      tylko jedna operacja na taśmie (ściągnięcie lub włożenie paczki)
// - trzeci - wielowartościowy - wspólny dla truckera i loaderów, aby trucker ściągnął
//      przynajmniej tyle paczek, ile trzeba, aby jakiś loader mógł dołożyć swoją paczkę


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

const char format[] = "%d-%m-%Y %H:%M:%S";
key_t belt_key, sem_message_key, sem_belt_operation_key, sem_belt_weight_key;
int belt_id, sem_message_id, sem_belt_operation_id, sem_belt_weight_id;
package *belt; // shared memory
char datetime[DATE_LENGTH];

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

void set_struct_sembuf(struct sembuf *str, int num, int op, int flag){
    str->sem_num = num;
    str->sem_op = op;
    str->sem_flg = flag;
}

void set_all_structs_sembuf(struct sembuf *s1, struct sembuf *s2, struct sembuf *s3, 
    struct sembuf *s4){
    set_struct_sembuf(s1, 0, -1, 0);
    set_struct_sembuf(s2, 0, 1, 0);
    set_struct_sembuf(s3, 0, -1, 0);
    set_struct_sembuf(s4, 0, 1, 0);
}

char *get_date_time(){ // datetime is statically allocated, but it's not a problem
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if(strftime(datetime, DATE_LENGTH, format, &tm) < 0) die_errno("strftime");
    printf("DT: %s\n", datetime);
    return datetime;
}

void print_package_details(package p){
    printf("Package: PID %d, %d, %s\n", p.workers_pid, p.weight, p.time_stamp);
}

#endif