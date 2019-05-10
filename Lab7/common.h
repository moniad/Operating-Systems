#ifndef __COMMON_H__
#define __COMMON_H__


#define PROJ_ID 7
#define SMH_SIZE 100

void die_errno(char *msg){
    perror(msg);
    exit(1);
}


#endif