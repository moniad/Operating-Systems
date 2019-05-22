#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h> // exit()
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>       /* For mode constants */
#include <mqueue.h> // mq_send()

#define MAX_MSG_SIZE 100
#define MAX_CL_COUNT 200
#define PROJ_ID 101
#define SERV_KEY 112
#define MSG_TYPES_COUNT 12
#define MAX_CMD_SIZE 100
#define MAX_MSG_COUNT 100


#define server_path "/server"

mqd_t serv_msqid; // id of server queue for clients to send their messages to server

typedef enum msgtype {
    // messages from client
    STOP = 12, LIST = 11, FRIENDS = 10, ADD = 9, DEL = 8,
    TO_ALL = 7, TO_FRIENDS = 6, TO_ONE = 5, ECHO = 4,
    // both
    INIT = 3, CL_PID = 2,
    // messages from server
    ANS = 1 //, END = 12,
} msgtype;

typedef struct msg{
    long mtype;
    int msqid; // used mainly to identify client
    int pid;
    char mtext[MAX_MSG_SIZE];
} msg;

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

#endif