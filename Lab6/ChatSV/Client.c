#include <stdio.h> // perror()
#include <sys/types.h> // msgget(), ftok(), msgsnd()
#include <sys/ipc.h> // msgget(), ftok()
#include <sys/msg.h> // msgget()
#include <sys/stat.h> // chmod()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include <signal.h>
#include <unistd.h> // sleep()
#include "Server.h"


/*
how to send STOP to Server??  send_msg(serv_msqid, STOP, key_string); <- is it ok?
do send jobs to server

*/

key_t key;
char key_string[MAX_MSG_SIZE];
int flags = IPC_CREAT | 0666;
int msqid; // queue for client to send their messages to server
int serv_msqid;
int command = IPC_RMID;
struct sigaction act; // handle SIGINT

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

    if(msgsnd(serv_msqid, &init_msg, strlen(init_msg.mtext)+1, IPC_NOWAIT) < 0){
       die_errno("msgsnd");
    }
}

void rm_queue(void){
    if(msgctl(msqid, command, NULL) < 0){
        die_errno("ipcrm");
    }
}


void SIGINThandler(int signum){
    printf("CLIENT: key: %d Received SIGINT. Sending STOP to SERVER...", key);
    send_msg(serv_msqid, STOP, key_string);
    printf("SENT. Now QUITTING...\n");
    exit(0);
}

void set_signal_handling(){
    act.sa_handler = SIGINThandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) < 0){
        die_errno("sigaction");
    }
    if(sigprocmask(SIG_SETMASK, &act.sa_mask, NULL) < 0){
        die_errno("sigprocmask");
    }
}

void get_key_string(){
    sprintf(key_string, "%d", key);
}

int main(void){
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
    set_signal_handling();
    // now send to server msgqueue the key of the newly created queue
    get_key_string();
    send_msg(serv_msqid, INIT, key_string);

    sleep(1);
    // receive message with ID
    msg rcvd_msg = receive_msg(msqid, ANS); 
    printf("RECEIVED ID (NUMBER IN THE QUEUE): %s\n", rcvd_msg.mtext);

    // send jobs to server
    // ...


    return 0;
}