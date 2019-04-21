#include <stdio.h> // perror()
#include <sys/types.h> // msgget(), ftok(), msgsnd()
#include <sys/ipc.h> // msgget(), ftok()
#include <sys/msg.h> // msgget()
#include <sys/stat.h> // chmod()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include <signal.h>
#include <unistd.h> // sleep()
#include <ctype.h> // isdigit()
#include <errno.h>
#include "Server.h"

#define MAX_FILE_SIZE 1000

/* todo:
- receiving messages from friends and so on
- SERVER STUFF eg. list() IS NOT TESTED FOR MULTIPLE CLIENTS!!!!
- some server stuff (handling SIGINT doesn't work on server side - server is not waiting untill everyone
sends STOP to them)

jak daję flagę IPC_NOWAIT, to muszę sprawdzać, czy msg nie jest NULL-em!
*/

// at the beginning I send client's PID in separate message
key_t key;
int flags = IPC_CREAT | IPC_EXCL | 0666; // IPC_EXCL - if queue already exists for key, msgget fails
char key_string[MAX_MSG_SIZE];
int msqid; // queue for client to send their messages to server
struct sigaction act; // handle SIGINT
char jobs_file_name[MAX_FILE_SIZE];
char *cmd;
char dlm[] = " \n\t";
char def_file_name[] = "jobs.txt";

void identify_cmd();

struct msg *receive_msg(int serv_msqid, int type, int flag){
    msg *rcvd_init_msg = malloc(sizeof(msg));
    int qid = type == INIT ? serv_msqid : msqid;
    if(msgrcv(qid, rcvd_init_msg, MAX_MSG_SIZE, type, flag) < 0){
        if(errno != ENOMSG)
            die_errno("client msgrcv");
        return NULL;
    }
    return rcvd_init_msg;
}

void send_msg(int type, char *message){
    msg mesg;
    mesg.mtype = type;
    strcpy(mesg.mtext, message);
    
    printf("SENDING: %s\n", mesg.mtext);

    int qid = type == INIT ? serv_msqid : msqid;
    if(msgsnd(qid, &mesg, strlen(mesg.mtext)+1, IPC_NOWAIT) < 0){
        die_errno("msgsnd");
    }
    if(type == INIT){
        mesg.mtype = CL_PID;
        char message[MAX_MSG_SIZE];
        sprintf(message, "%d", getpid());
        strcpy(mesg.mtext, message);
        if(msgsnd(qid, &mesg, strlen(message), IPC_NOWAIT) < 0){
            die_errno("pid msgsnd");
        }
    }
}

void rm_queue(void){
    if(msgctl(msqid, command, NULL) < 0){
        die_errno("ipcrm");
    }
}

void SIGINThandler(int signum){
    printf("CLIENT: key: %d Received SIGINT. Sending STOP to SERVER...", key);
    send_msg(STOP, key_string);
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

size_t get_file_size(char *file_name){
    FILE *file = fopen(file_name, "r");
    if(!file) die_errno("Couldn't open file!\n");
    //SEEK_END moves file pointer position to the end of file
    fseek(file, 0, SEEK_END);
    size_t size = (size_t) ftell(file); //returns the position of file ptr
    fclose(file);
    return size;
}

char *get_file_content(char *path_to_file){
    size_t file_size = get_file_size(path_to_file);
    char *file_content = calloc(file_size+1, sizeof(char));
    // reading file_content
    FILE *file = fopen(path_to_file, "r");

    if(fread(file_content, sizeof(char), file_size, file) != file_size)
        die_errno("Sth went wrong during reading data from file");
    fclose(file);
    return file_content;
}

void stop(){
    printf("STOP\n");
    send_msg(STOP, key_string);
    exit(0);
}

void list(){
    printf("LIST\n");
    send_msg(LIST, key_string); 
    printf("\n");
}

char *concat_message(char *message){
    strcpy(message, cmd);
    strcat(message, " ");
    cmd = strtok(NULL, dlm);
    while(cmd && isdigit(cmd[0])){
        strcat(message, cmd);
        strcat(message, " ");
        cmd = strtok(NULL, dlm);
    }
    return message;
}
void echo(){
    char *message = strtok(NULL, dlm);
    send_msg(ECHO, message);
    msg *rcvd_msg = receive_msg(msqid, ECHO, 0);
    printf("CLIENT ECHO: %s\n\n", rcvd_msg->mtext);
}

void add(int type){
    switch(type){
        case ADD:
            printf("ADD   ");
            break;
        case FRIENDS:
            printf("FRIENDS   ");
            break;
        case DEL:
            printf("DEL   ");
            break;
        default:
            break;
    }
    cmd = strtok(NULL, dlm);
    if(!cmd || !isdigit(cmd[0])){ // no IDs given ==> not adding anything || sending empty msg
        if(type == FRIENDS) {
            printf("CLEARING LIST...\n\n");
            send_msg(FRIENDS, "");
        }
        else if(type == ADD) printf("No client IDs to add\n\n");
        else printf("No clients to delete!\n\n");
        if(cmd) identify_cmd(); // go on, cause we read it, but didn't use it!
        return;
    }
    char *message = malloc(MAX_MSG_SIZE * sizeof(char));
    concat_message(message);
    send_msg(type, message);
    // is not digit any more
    if(cmd) identify_cmd(); // go on, cause we read it, but didn't use it!
    // free(message); <- is it OK????
}

void to_all_or_friends(int type){
    printf("TO ");
    if(type == TO_ALL) printf("ALL: ");
    else printf("FRIENDS: ");
    cmd = strtok(NULL, dlm);
    if(!cmd) die_errno("incorrect input. to_all_or_friends");
    send_msg(type, cmd);
}

void to_one(){
    printf("TO_ONE\n");
    cmd = strtok(NULL, dlm);
    if(!cmd) die_errno("incorrect input. to_one");
    char *message = malloc(MAX_MSG_SIZE * sizeof(char));
    strcpy(message, cmd);
    strcat(message, " ");
    cmd = strtok(NULL, dlm);
    if(!cmd) die_errno("incorrect input. to_one");
    strcat(message, cmd);
    send_msg(TO_ONE, message);
}

void identify_cmd(){
    sleep(2);
    if(strcmp(cmd, "STOP") == 0) stop();
    else if(strcmp(cmd, "LIST") == 0) list();
    else if(strcmp(cmd, "FRIENDS") == 0) add(FRIENDS);
    else if(strcmp(cmd, "ECHO") == 0) echo();
    else if(strcmp(cmd, "ADD") == 0) add(ADD);
    else if(strcmp(cmd, "DEL") == 0) add(DEL);
    else if(strcmp(cmd, "TO_ALL") == 0) to_all_or_friends(TO_ALL);
    else if(strcmp(cmd, "TO_FRIENDS") == 0) to_all_or_friends(TO_FRIENDS);
    else if(strcmp(cmd, "TO_ONE") == 0) to_one();
    else printf("Command not recognized!\n");
}

void send_jobs_to_server(){
    // read from file using strtok
    char *file_content = get_file_content(jobs_file_name);
    if(!file_content) die_errno("Empty jobs file!");
    printf("FILE CONTENT %s\n\n", file_content);
    cmd = strtok(file_content, dlm);
    if(!cmd) die_errno("NULL cmd");
    identify_cmd();
    while((cmd = strtok(NULL, dlm)) != NULL)
        identify_cmd();
}

void parse_input(int argc, char **argv){
    if(argc < 2)
        strcpy(jobs_file_name, def_file_name);
    else
        strcpy(jobs_file_name, argv[1]);
}

int main(int argc, char **argv){
    parse_input(argc, argv);

    // getenv() - searches the env list in order to find the env variable name 
    if((key = ftok(getenv("HOME"), PROJ_ID) % 1000) < 0) die_errno("ftok");
    printf("key: %d\n", key);
    // get server msqid:
    if((serv_msqid = msgget(SERV_KEY, 0666)) < 0) die_errno("msget");
    printf("SERVER ID: %d\n", serv_msqid);
    // get your msqid
    if((msqid = msgget(key, flags)) < 0) die_errno("msgget");
    if(atexit(rm_queue) < 0) die_errno("atexit");
    set_signal_handling();
    // now send to server msgqueue the key of the newly created queue
    get_key_string();
    send_msg(INIT, key_string);

    sleep(1);
    // receive message with ID
    msg *rcvd_msg = receive_msg(msqid, ANS, 0); 
    printf("RECEIVED ID (NUMBER IN THE QUEUE): %s\n", rcvd_msg->mtext);


////////////////////////////////////////
// -- ECHO works on both sides :)
    send_jobs_to_server();
    
    return 0;
}