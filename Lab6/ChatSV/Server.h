#ifndef SERVER_H
#define SERVER_H

#define MAX_MSG_SIZE 100
#define MAX_CL_COUNT 200
#define PROJ_ID 101
#define SERV_KEY 112

typedef enum msgtype {
    STOP = 1, LIST = 2, FRIENDS = 3, ADD = 4, DEL = 5,
    TO_ALL = 6, TO_FRIENDS = 7, TO_ONE = 8, ECHO = 9,
    // server messages
    INIT = 10, ANS = 11, END = 12
} msgtype;

#endif