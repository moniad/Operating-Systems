#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h> // perror()
#include <stdlib.h> // exit()

#define PROJ_ID 7
#define SMH_SIZE 100

struct sembuf take, give;
char *belt; // shared memory

typedef struct package{
    pid_t workers_pid;
    int weight;
    
} package;

/* but what should shared memory look like??? */

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

#endif