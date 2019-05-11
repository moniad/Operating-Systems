#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h> // perror()
#include <stdlib.h> // exit()
#include <sys/sem.h> // struct sembuf

#define PROJ_ID 7
#define SMH_SIZE 100
#define DATE_LENGTH 30

struct sembuf take, give;
char *belt; // shared memory
char datetime[DATE_LENGTH];

typedef struct package{
    pid_t workers_pid;
    int weight;
    char time_stamp[DATE_LENGTH];
} package;

/* but what should shared memory look like??? */

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

#endif