#include <stdio.h> // perror()
#include <sys/types.h> // msgget(), ftok(), msgsnd()
#include <sys/ipc.h> // msgget(), ftok()
#include <sys/msg.h> // msgget()
#include <sys/stat.h> // chmod()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include "Server.h"

#define SERV_ANS 999

/* TYPES OF MESSAGES:
1 - INIT

*/

int flags = IPC_CREAT | 0666;

typedef struct msg{
    long mtype;
    char mtext[MAX_MSG_SIZE];
} msg;

typedef struct msgbuf{
    long mtype;
    char *mtext;
} msgbuf;

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

struct msg receive_msg(int serv_msqid, int type) { //, int key){
    msg rcvd_init_msg;
    if(msgrcv(serv_msqid, &rcvd_init_msg, MAX_MSG_SIZE, type, 0) < 0){
        die_errno("client msgrcv");
    }
    return rcvd_init_msg;
}

int main(void){
    key_t key;
    int serv_msqid;
    int msqid;
    // getenv() - searches the env list in order to find the env variable name 
    if((key = ftok(getenv("HOME"), PROJ_ID)) < 0){
        perror("ftok");
    }
    printf("key: %d\n", key);

    if((msqid = msgget(key, flags)) < 0){ // queue for clients to send their messages to server
        die_errno("msgget");
    }

    msg init_msg;
    init_msg.mtype = INIT;
    sprintf(init_msg.mtext, "%d", key);
    
    printf("SENDING: %s\n", init_msg.mtext);

    // get server msqid:
    if((serv_msqid = msgget(SERV_KEY, 0666)) < 0){
        die_errno("msget");
    }
    // now send to server msgqueue the key of the newly created queue
    if(msgsnd(serv_msqid, &init_msg, strlen(init_msg.mtext)+1, IPC_NOWAIT) < 0){
       die_errno("msgsnd");
    }
    printf("SENT\n");

    // //-------------------
    // // receive message
    // msg rcvd_msg = receive_msg(msqid, SERV_ANS); 
    // printf("RECEIVED NUMBER IN THE QUEUE: %s\n", rcvd_msg.mtext);

    // // struct msgbuf *msg = malloc(sizeof(msgbuf*));
    // // msg->mtext = malloc(MAX_MSG_SIZE * sizeof(char));

    // // msg->mtype = 3;
    // // msg->mtext

    return 0;
}