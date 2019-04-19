#include <stdio.h> // perror()
#include <sys/types.h> // msgget(), ftok(), msgsnd()
#include <sys/ipc.h> // msgget(), ftok()
#include <sys/msg.h> // msgget()
#include <sys/stat.h> // chmod()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include "Server.h"


/*

do send jobs to server

*/

key_t key;
int flags = IPC_CREAT | 0666;
int msqid; // queue for client to send their messages to server
int command = IPC_RMID;

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

void send_msg(int serv_msqid, int type, char *message){
    msg init_msg;
    init_msg.mtype = type;
    strcpy(init_msg.mtext, message);
    
    printf("SENDING: %s\n", init_msg.mtext);

    // now send to server msgqueue the key of the newly created queue
    if(msgsnd(serv_msqid, &init_msg, strlen(init_msg.mtext)+1, IPC_NOWAIT) < 0){
       die_errno("msgsnd");
    }
}

void rm_queue(void){
    if(msgctl(msqid, command, NULL) < 0){
        die_errno("ipcrm");
    }
}


int main(void){
    int serv_msqid;
    // getenv() - searches the env list in order to find the env variable name 
    if((key = ftok(getenv("HOME"), PROJ_ID) % 1000) < 0){
        die_errno("ftok");
    }
    printf("key: %d\n", key);
    // get server msqid:
    if((serv_msqid = msgget(SERV_KEY, 0666)) < 0){
        die_errno("msget");
    }
    printf("SERVER ID: %d\n", serv_msqid);
    // get your msqid
    if((msqid = msgget(key, flags)) < 0){
        die_errno("msgget");
    }
    if(atexit(rm_queue) < 0){
        die_errno("atexit");
    }
    

    char message[MAX_MSG_SIZE];
    sprintf(message, "%d", key);
    send_msg(serv_msqid, INIT, message);

    // receive message with ID
    msg rcvd_msg = receive_msg(msqid, ANS); 
    printf("RECEIVED ID (NUMBER IN THE QUEUE): %s\n", rcvd_msg.mtext);

    // send jobs to server

    return 0;
}