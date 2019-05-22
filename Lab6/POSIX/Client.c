#define _GNU_SOURCE

#include <stdio.h> // perror()
#include <sys/stat.h> // chmod()
#include <stdlib.h> // malloc(), getenv()
#include <string.h>
#include <signal.h>
#include <unistd.h> // sleep()
#include <ctype.h> // isdigit()
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>       /* For mode constants */
#include <mqueue.h> // mq_send()
#include "Server.h"

#define MAX_FILE_SIZE 1000

/* todo:
- receiving messages from friends and so on - I can do fork() and do receive_msg() from child!!!
- SERVER STUFF eg. request_list() IS NOT TESTED FOR MULTIPLE CLIENTS!!!!
- some server stuff (handling SIGINT doesn't work on server side - server is not waiting untill everyone
sends STOP to them)

jak daję flagę IPC_NOWAIT, to muszę sprawdzać, czy msg nie jest NULL-em!
*/

// at the beginning I send client's PID in separate message
key_t key;
int flags = O_RDONLY | O_CREAT;
char key_string[MAX_MSG_SIZE];
mqd_t msqid; // queue for client to send their messages to server
struct sigaction act; // handle SIGINT
char jobs_file_name[MAX_FILE_SIZE];
char *cmd;
char dlm[] = " \n\t";
char def_file_name[] = "jobs.txt";

void identify_cmd();

struct msg receive_msg(int type, int *priority){ // always receiving msg to client's queue
    if(*priority < 0) die_errno("Priority must be > 0!");
    msg rcvd_init_msg;
    if(mq_receive(msqid, (char *) &rcvd_init_msg, MAX_MSG_SIZE, (unsigned int*) priority) < 0){
        die_errno("client mq_receive");
    }
    return rcvd_init_msg;
}

void send_msg(int type, char *message){ // always sending to server
    msg mesg;
    mesg.mtype = type;
    mesg.msqid = msqid;
    mesg.pid = getpid();

    strcpy(mesg.mtext, message);
    
    printf("SENDING: %s\n", mesg.mtext);
    
    if(mq_send(serv_msqid, (char*) &mesg, MAX_MSG_SIZE, 1) < 0) die_errno("msgsnd");
}

// void rm_queue(void){
//     if(msgctl(msqid, command, NULL) < 0){
//         die_errno("ipcrm");
//     }
// }

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

void request_stop(){
    printf("STOP\n");
    send_msg(STOP, "");
    // sleep(10);
    exit(0);
}

void request_list(){
    printf("LIST\n");
    send_msg(LIST, ""); 
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
void request_echo(){
    char *message = strtok(NULL, dlm);
    send_msg(ECHO, message);
    msg rcvd_msg = receive_msg(ECHO, NULL);
    printf("CLIENT ECHO: %s\n\n", rcvd_msg.mtext);
}

void request_add(int type){
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
    if(!cmd || !isdigit(cmd[0])){ // no IDs given ==> not request_adding anything || sending empty msg
        if(type == FRIENDS) {
            printf("CLEARING LIST...\n\n");
            send_msg(FRIENDS, "");
        }
        else if(type == ADD) printf("No client IDs to request_add\n\n");
        else printf("No clients to delete!\n\n");
        if(cmd) identify_cmd(); // go on, cause we read it, but didn't use it!
        return;
    }
    char *message = malloc(MAX_MSG_SIZE * sizeof(char));
    concat_message(message);
    send_msg(type, message);
    // is not digit any more
    if(cmd) identify_cmd(); // go on, cause we read it, but didn't use it!
    free(message); //NOPE - freeing on other side
}

void request_to_all_or_friends(int type){
    printf("TO ");
    if(type == TO_ALL) printf("ALL: ");
    else printf("FRIENDS: ");
    cmd = strtok(NULL, dlm);
    if(!cmd) die_errno("incorrect input. request_to_all_or_friends");
    send_msg(type, cmd);
}

void request_to_one(){
    printf("TO_ONE\n");
    cmd = strtok(NULL, dlm);
    if(!cmd) die_errno("incorrect input. request_to_one");
    char *message = malloc(MAX_MSG_SIZE * sizeof(char));
    strcpy(message, cmd);
    strcat(message, " ");
    cmd = strtok(NULL, dlm);
    if(!cmd) die_errno("incorrect input. request_to_one");
    strcat(message, cmd);
    send_msg(TO_ONE, message);
    free(message);
}

void identify_cmd(){
    sleep(2);
    if(strcmp(cmd, "STOP") == 0) request_stop();
    else if(strcmp(cmd, "LIST") == 0) request_list();
    else if(strcmp(cmd, "FRIENDS") == 0) request_add(FRIENDS);
    else if(strcmp(cmd, "ECHO") == 0) request_echo();
    else if(strcmp(cmd, "ADD") == 0) request_add(ADD);
    else if(strcmp(cmd, "DEL") == 0) request_add(DEL);
    else if(strcmp(cmd, "TO_ALL") == 0) request_to_all_or_friends(TO_ALL);
    else if(strcmp(cmd, "TO_FRIENDS") == 0) request_to_all_or_friends(TO_FRIENDS);
    else if(strcmp(cmd, "TO_ONE") == 0) request_to_one();
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
    free(file_content);
}

void parse_input(int argc, char **argv){
    if(argc < 2)
        strcpy(jobs_file_name, def_file_name);
    else
        strcpy(jobs_file_name, argv[1]);
}

void login_client(){
    send_msg(INIT, "");
    msg message = receive_msg(INIT, NULL);
    printf("Client: Received ID %s\n", message.mtext);
}

int main(int argc, char **argv){
    parse_input(argc, argv);
    set_signal_handling();

    char priv_client_path[20];
    sprintf(priv_client_path, "/%d", getpid());
    if((serv_msqid = mq_open(server_path, O_WRONLY)) < 0) die_errno("client: serv_msqid");

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSG_COUNT;
    attr.mq_msgsize = MAX_MSG_SIZE;
    if((msqid = mq_open(priv_client_path, flags, 0666, &attr)) < 0) die_errno("client msqid");

    login_client();

    send_jobs_to_server();
    
    return 0;
}