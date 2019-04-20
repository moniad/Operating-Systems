#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h> // exit()
#include <unistd.h>
#include <sys/ipc.h> // flags

#define MAX_MSG_SIZE 100
#define MAX_CL_COUNT 200
#define PROJ_ID 101
#define SERV_KEY 112
#define MSG_TYPES_COUNT 12

int command = IPC_RMID; // for atexit()
int serv_msqid; // id of server queue for clients to send their messages to server

typedef enum msgtype {
    STOP = 1, LIST = 2, FRIENDS = 3, ADD = 4, DEL = 5,
    TO_ALL = 6, TO_FRIENDS = 7, TO_ONE = 8, ECHO = 9,
    // server messages
    INIT = 10, ANS = 11, END = 12,
} msgtype;

typedef struct msg{
    long mtype;
    char mtext[MAX_MSG_SIZE];
} msg;

void die_errno(char *msg){
    perror(msg);
    exit(1);
}


#endif