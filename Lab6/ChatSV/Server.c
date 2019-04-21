/*Napisz prosty chat typu klient-serwer, w którym komunikacja zrealizowana jest za 
pomocą kolejek komunikatów - jedna, na zlecenia klientów dla serwera, druga, prywatna,
na odpowiedzi.

Wysyłane zlecenia klientow do serwera mają zawierać 
1) rodzaj zlecenia jako rodzaj komunikatu
2) informację od którego klienta zostały wysłane (ID klienta).

W odpowiedzi rodzajem komunikatu ma być 
1) informacja identyfikująca czekającego na nią klienta.

komunikaty z:
- stdin
- albo pliku
*/

#include <stdio.h> // perror()
#include <sys/types.h> // msgget(), ftok()
#include <sys/ipc.h> // msgget(), ftok()
#include <sys/msg.h> // msgget()
#include <stdlib.h> // system()
#include <string.h>
#include <signal.h> // sigaction(), sigprocmask()
#include <unistd.h> // sleep()
#include <errno.h> // errno
#include <time.h>
#include "Server.h"

#define DATE_LENGTH 30
#define MAX_ID_LENGTH 10

typedef struct client{
    int clientID;
    pid_t pid;
} client;
// key_t clients[MAX_CL_COUNT]; // client keys -- maybe IDs instead???
client clients[MAX_CL_COUNT];
char *friendsIDs;
int friends_count = 0;
int clientsInd = 0;
int flags = IPC_CREAT | 0666;
/*
clients
change login_client()
SIGINThandler() - to check - receive stop from all clients

Serwer może wysłać do klientów komunikaty:
- inicjujący pracę klienta (kolejka główna serwera)
- wysyłający odpowiedzi do klientów (kolejki klientów)
- informujący klientów o zakończeniu pracy serwera - po wysłaniu takiego sygnału i odebraniu wiadomości STOP od wszystkich klientów serwer usuwa swoją kolejkę i kończy pracę. (kolejki klientów)

*/
int clientCount;
struct sigaction act;

struct msg *receive_msg(int msqid, int type, int msgflag){
    msg *rcvd_init_msg = malloc(sizeof(msg));
    if(msgrcv(msqid, rcvd_init_msg, MAX_MSG_SIZE, type, msgflag) < 0){
        if(errno != ENOMSG) die_errno("server msgrcv");
        return NULL;
    }
    return rcvd_init_msg;
}

//change it
void login_client(){
    if(clientCount == MAX_CL_COUNT){
        fprintf(stderr, "%s", "Cannot login new client. Too many clients logged in. Wait");
    }
    int cl_msqid;
    // receive message
    msg *rcvd_init_msg = receive_msg(serv_msqid, INIT, 0);
    
    printf("RECEIVED: %s\n", rcvd_init_msg->mtext);
    key_t cl_key = (int) strtol(rcvd_init_msg->mtext, NULL, 10);
    //-------------------
    // answer 
    // client using their queue. we know its key because it was sent in the init msg
    if((cl_msqid = msgget(cl_key, flags)) < 0){
        die_errno("msget, answering client");
    }
    printf("CLIENT ID: %d\n", cl_msqid);
    clients[++clientCount].clientID = cl_msqid;
    // getting the PID of the process who SENT their key just before a while
    rcvd_init_msg = receive_msg(serv_msqid, CL_PID, 0);
    clients[clientCount].pid = (int) strtol(rcvd_init_msg->mtext, NULL, 10);
    printf("RECEIVED && CONVERTED PID %d\n", clients[clientCount].pid);
    msg answer;
    answer.mtype = ANS;
    sprintf(answer.mtext, "%d", cl_msqid);
    printf("ANSWER WITH CLIENTID: %s\n", answer.mtext);
        // send answer
    if(msgsnd(cl_msqid, &answer, strlen(answer.mtext), IPC_NOWAIT) < 0){
       die_errno("msgsnd, answering client");
    }

    printf("MSG SENT TO CLIENT\n");
}

void rm_queue(void){
    if(msgctl(serv_msqid, command, NULL) < 0){
        die_errno("ipcrm");
    }
}

void rm_client_queue(int cl_msqid){
    if(msgctl(cl_msqid, command, NULL) < 0){
        die_errno("ipcrm client's queue");
    }
}

void SIGINThandler(int signum){
    printf("SERVER: Received SIGINT. Quiting...");
    // sending to all clients SIGINT && waiting for all clients to send STOP
    for(int i = 0; i < clientCount; i++){
        kill(clients[i].pid, SIGINT);
        sleep(1);
        if(msgrcv(clients[i].clientID, NULL, MAX_MSG_SIZE, STOP, IPC_NOWAIT) < 0)
            die_errno("didn't receive STOP message from client");
        rm_client_queue(clients[i].clientID);
    }
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

void send_msg(int cl_msqid, int type, char *message, char *errno_msg){
    msg mesg;
    mesg.mtype = type;
    strcpy(mesg.mtext, message);

    printf("SENDING: %s\n", mesg.mtext);

    if(msgsnd(cl_msqid, &mesg, strlen(mesg.mtext), IPC_NOWAIT) < 0)
        die_errno(errno_msg);
}

void echo(int cl_msqid, char *string){
    if(!string) die_errno("empty string in echo");
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char * datetime = malloc(DATE_LENGTH * sizeof(char));
    if(strftime(datetime, DATE_LENGTH, "%d-%m-%Y %H:%M:%S", &tm) == 0)
        die_errno("strftime");
    char *res_string = malloc(strlen(string) + DATE_LENGTH);
    strcpy(res_string, string);
    strcat(res_string, " ");
    strcat(res_string, datetime);
    send_msg(cl_msqid, ECHO, res_string, "msgsnd, ECHO");
}

void list(){
    printf("LISTING active clients:\n");
    for(int i = 0; i < MAX_CL_COUNT; i++)
        if(clients[i].clientID != -1)
            printf("%d ", clients[i].clientID);
}

void stop(int cl_msqid){
    int i; // client index in the clients array - needed to put -1 there
    // what signals that client is no longer connected to the server
    for(i = 0; i < MAX_CL_COUNT; i++)
        if(clients[i].clientID == cl_msqid) break;
    if(clients[i].clientID != cl_msqid)
        die_errno("STOP: Clients ID not found in the array");
    rm_client_queue(clients[i].clientID);
    clients[i].clientID = clients[i].pid = -1;
}

int is_client_connected(char *clientID){
    int cl_ID = (int) strtol(clientID, NULL, 10);
    for(int i = 0; i < MAX_CL_COUNT; i++){
        if(clients[i].clientID == cl_ID)
            return 1;
    }
    return 0;
}
void set_new_friends(char *list){ // there won't be more clients connected 
    // than max clients because either they won't be connected, or they will already
    // be connected
    // cleaning the old list:
    strcpy(friendsIDs, "");
    int new_friends_count = 0;
    // counted this way because strtok destroys the string passed as an argument
    char *copy_of_list = malloc((strlen(list)+1) * sizeof(char));
    strcpy(copy_of_list, list);
    char *oneFriend = strtok(copy_of_list, " ");
    while(oneFriend){
        /* TURNED OFF JUST FOR A WHILE!!!!!!!!!!!!
        if(!is_client_connected(oneFriend)){
            printf("Client not connected: %s\n", copy_of_list);
        }
        else{*/
            new_friends_count++;
            strcat(friendsIDs, " ");
            strcat(friendsIDs, oneFriend);
       /* } */
        oneFriend = strtok(NULL, " ");
    }
    friends_count = new_friends_count;
}

// I should check there if the no of IDs in the list isn't > MAX_CLIENT_COUNT
void friends(char *list_of_friendsIDs){ // empty message is sent if FRIENDS
    // (without IDs) is sent
    // printf("\n\nFRIENDS. friends count before: %d\n", friends_count);
    // printf("FRIENDS IDS before: %s vs ", friendsIDs);
    set_new_friends(list_of_friendsIDs);
    // printf("VS after: %d\n", friends_count);
    // printf("after: %s\n", friendsIDs);
}

void add(char *list_of_friends){
    // printf("ADD: before: %s vs ", friendsIDs);
    strcat(friendsIDs, " ");
    strcat(friendsIDs, list_of_friends);
    // printf("after: %s\n", friendsIDs);
}


///
//
// TO TEST!!!!!!!!!!!!
//
// UNCOMMENT IF IN SET_NEW_FRIENDS!!!!!!!!!!!!!!!!!!!!
///
//
//

void del(char *friends_to_rmv){
    char *newFriendsList = calloc(MAX_CL_COUNT * (1 + MAX_ID_LENGTH), sizeof(char));
    char *curFriend;
    char *friendToDel = strtok(friends_to_rmv, " ");
    while(friendToDel){
        curFriend = strtok(friendsIDs, " ");
        while(curFriend){
            if(curFriend != friendToDel){

            }
        }
        if(!curFriend){

        }
    }
}


void init_array_and_vars(){
    friendsIDs = calloc(MAX_CL_COUNT * (1 + MAX_ID_LENGTH), sizeof(char)); // counting the spaces between IDs
    clientCount = -1;
    friends_count = 0;
    for(int i = 0; i < MAX_CL_COUNT; i++)
        clients[i].clientID = clients[i].pid = -1;
}

int main(void){
    init_array_and_vars();
    set_signal_handling();

    // give the process read & write permission
    if((serv_msqid = msgget(SERV_KEY, flags)) < 0){
        die_errno("msget");
    }
    if(atexit(rm_queue) < 0){
        die_errno("atexit");
    }

  ///////////////////////////////////////////////////////////////  
    
    // ------ ok
    login_client(); // may need changes
    // --- changing it
    msg *rcvd_msg;
    int clientID;
    sleep(2);
    friends("234 123 456 777");
    add("222");
    del("123");
    ///////////////////////////////////////////////////////////////
    // while(1){ // serving clients according to clientsInd
    //     printf("\nFirst things first\n");
    //     for(int i = 1; i <= 3; i++) // types: STOP, LIST, FRIENDS are served first
    //         for(int j = 0; j < MAX_CL_COUNT; j++){
    //             printf("j: %d, ", j);
    //             while((rcvd_msg = receive_msg(clients[j].clientID, i, IPC_NOWAIT)) != NULL){
    //                 printf("RECEIVED MESSAGE: %s\n", rcvd_msg->mtext);
    //                 sleep(1);
    //                 /*
                

    //                 doddodododo sth


    //                 */
    //             }
    //         }
    
    //     // then the remaining requests are served
    //     printf("\nThen the rest\n");
    //     // serving client whose id == clients[clientsInd].clientID
    //         // receiving ALL messages of ANY TYPE because STOP message cannot come before INIT
    //         // It would have been received if it had come.
    //     clientID = clients[clientsInd].clientID;
    //     while((rcvd_msg = receive_msg(clientID, 0, IPC_NOWAIT)) != NULL){
    //         printf("RECEIVED MESSAGE: %s\n", rcvd_msg->mtext);
    //             /*


    //             doddodododo sth


    //             */

    //     }
    //     clientsInd++; // serving next client;
    // }


    // there is a segfault when i do it for the second time!!!


    // while(1){
        // msg *message = receive_msg(clients[0].clientID, 0, IPC_NOWAIT);
        // if(!message) printf("NOOOOO");
        // printf("message text: %s", message->mtext);
    // }
    return 0;
}

// works :)
// - with all: echo(clients[0].clientID, "heeeeej");
// - with one client (for sure): list();

/*  receive stop from Client. LOGGING IN A CLIENT SHOULD BE CHANGED (WE CAN FIND
 PLACES WITH -1 THERE? ?? ? IS IT OK???)
    
    // clientID
    int clientInd = -1;
    for(int i = 0; i < clientCount; i++)
        if(clientID == clientsID[i]){
            clientInd = i;
            break;
        }
    receive_msg(clientsID[clientInd], STOP);
    // remove client's ID from the table
    clientsID[clientInd] = -1;
    rm_client_queue(clientID);

*/ // getting stop from client