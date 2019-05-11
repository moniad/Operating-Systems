#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h> // perror()
#include <stdlib.h> // exit()

#define PROJ_ID 7
#define SMH_SIZE 100

struct sembuf take, give;
char *belt; // shared memory

/* but what should shared memory look like??? */

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

#endif