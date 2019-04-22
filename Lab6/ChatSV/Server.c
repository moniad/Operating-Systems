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

Serwer może wysłać do klientów komunikaty:
- inicjujący pracę klienta (kolejka główna serwera)
- wysyłający odpowiedzi do klientów (kolejki klientów)
- informujący klientów o zakończeniu pracy serwera - po wysłaniu takiego sygnału i odebraniu wiadomości STOP od wszystkich klientów serwer usuwa swoją kolejkę i kończy pracę. (kolejki klientów)

*/
int clientCount;
struct sigaction act;
void SIGINThandler(int signum);

char *get_date_time(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char * datetime = malloc(DATE_LENGTH * sizeof(char));
    if(strftime(datetime, DATE_LENGTH, "%d-%m-%Y %H:%M:%S", &tm) == 0)
        die_errno("strftime");
    return datetime;
}

struct msg *receive_msg(int msqid, int type, int msgflag){
    msg *rcvd_init_msg = malloc(sizeof(msg));
    if(msgrcv(msqid, rcvd_init_msg, MAX_MSG_SIZE, type, msgflag) < 0){
        if(errno != ENOMSG) die_errno("server msgrcv");
        return NULL;
    }
    return rcvd_init_msg;
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

    if(msgsnd(cl_msqid, &mesg, MAX_MSG_SIZE, IPC_NOWAIT) < 0)
        die_errno(errno_msg);
}

void echo(int cl_msqid, char *string){
    if(!string) die_errno("empty string in echo");
    char *res_string = malloc(strlen(string) + DATE_LENGTH);
    strcpy(res_string, string);
    strcat(res_string, " ");
    strcat(res_string, get_date_time());
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
    // it signals that client is no longer connected to the server
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

int is_client_a_friend(char *clientID){
    // counted this way because strtok destroys the string passed as an argument
    char *copy_of_friends = malloc(MAX_CL_COUNT * (MAX_ID_LENGTH+1) * sizeof(char));
    strcpy(copy_of_friends, friendsIDs);
    char *friendID;
    while((friendID = strtok_r(copy_of_friends, " ", &copy_of_friends)) != NULL)
        if(strcmp(friendID, clientID) == 0)
            return 1;
    // free(copy_of_friends); <- FORBIDDEN!!!
    return 0;
}

void set_new_friends(int type, char *list){ // there won't be more clients connected 
    // than max clients because either they won't be connected, or they will already
    // be connected
    int new_friends_count;
    if(type == FRIENDS){
        // cleaning the old list:
        strcpy(friendsIDs, "");
        new_friends_count = 0;
    }
    else if(type == ADD)
        new_friends_count = friends_count;
    else die_errno("Incorrect type passed to set_new_friends");
    // counted this way because strtok destroys the string passed as an argument
    // printf("NEW FRIENDS LIST: %s\n", list);
    char *copy_of_list = malloc(MAX_CL_COUNT * (MAX_ID_LENGTH+1) * sizeof(char));
    strcpy(copy_of_list, list);
    char *oneFriend;

    while((oneFriend = strtok_r(copy_of_list, " ", &copy_of_list)) != NULL){
        // printf("ADDING (OR NOT) ONE FRIEND: %s\n", oneFriend);
        if(!is_client_connected(oneFriend)){
            printf("Client not connected: %s\n", oneFriend);
        }
        else if(is_client_a_friend(oneFriend)){
            printf("Client %s is already among friends\n", oneFriend);
        }
        else{
            new_friends_count++;
            strcat(friendsIDs, " ");
            strcat(friendsIDs, oneFriend);
        }
        // printf("AFTER ALL One friend: %s AND LIST %s\n", oneFriend, friendsIDs);
    }
    friends_count = new_friends_count;
}

void add(char *list_of_friends){ // includes checking if given friend is only once
    // on a list
    printf("ADD: before: %s vs ", friendsIDs);
    set_new_friends(ADD, list_of_friends);
    printf("after: %s\n", friendsIDs);
}

void friends(char *list_of_friends){
    printf("FRIENDS: before: %s vs ", friendsIDs);
    set_new_friends(FRIENDS, list_of_friends);
    printf("after: %s\n", friendsIDs);
}

int get_no_of_friends(char *friends_IDs_list){
    int friends = 0;
    char *copy_of_fr_IDs_list = calloc(strlen(friends_IDs_list)+1, sizeof(char));
    strcpy(copy_of_fr_IDs_list, friends_IDs_list);
    char *friend;
    while((friend = strtok_r(copy_of_fr_IDs_list, " ", &copy_of_fr_IDs_list)) != NULL)
        friends++;
    return friends;
}

void del(char *friends_to_rmv){ // creating a new list. appending clients whose IDs
    // are not on the friends_to_rmv list
    char *newFriendsList = calloc(MAX_CL_COUNT * (1 + MAX_ID_LENGTH), sizeof(char));
    char *copy_of_friends = calloc(MAX_CL_COUNT * (1 + MAX_ID_LENGTH), sizeof(char));
    strcpy(copy_of_friends, friendsIDs);
    char *curFriend;
    char *friendToDel;
    int rmvd;
    while((friendToDel = strtok_r(friends_to_rmv, " ", &friends_to_rmv)) != NULL){
        rmvd = 0;
        while((curFriend = strtok_r(copy_of_friends, " ", &copy_of_friends)) !=NULL){
            if(strcmp(curFriend, friendToDel) != 0){
                strcat(newFriendsList, " ");
                strcat(newFriendsList, curFriend);
            }
            else rmvd = 1;
        }
        if(!rmvd) printf("Client with ID = %s was not on the list of friends!\n", friendToDel);
        // printf("new friends list: %s\n", newFriendsList);
        strcpy(copy_of_friends, newFriendsList);
    }
    friends_count = get_no_of_friends(newFriendsList);
    printf("friends count: %d\n", friends_count);
    strcpy(friendsIDs, newFriendsList);
}

void to_all_or_friends(int type, int cl_msqid, char *string){
    if(!string) die_errno("empty string in to_all()");
    
    char *res_string = malloc(MAX_MSG_SIZE);
    strcpy(res_string, "Client with ID = ");

    char *msqid_str = malloc(MAX_ID_LENGTH);
    sprintf(msqid_str, "%d", cl_msqid);

    strcat(res_string, msqid_str);
    strcat(res_string, " wrote \"");
    strcat(res_string, string);
    strcat(res_string, "\" on ");
    strcat(res_string, get_date_time());
    printf("FINAL MESSAGE: %s\n", res_string);

    if(type == TO_ALL){
        for(int i = 0; i < MAX_CL_COUNT; i++)
            if(clients[i].clientID != -1)
                send_msg(clients[i].clientID, TO_ALL, res_string, "msgsnd, to_all()");
    }
    else if(type == TO_FRIENDS){
        char *copy_of_friends = malloc(MAX_CL_COUNT * (MAX_ID_LENGTH + 1) * sizeof(char));
        strcpy(copy_of_friends, friendsIDs);
        char *friend;
        while((friend = strtok_r(copy_of_friends, " ", &copy_of_friends)) != NULL){
            // printf("friend ID: %s\n", friend);
            send_msg((int) strtol(friend, NULL, 10), TO_FRIENDS, res_string, "msgsnd, to_friends()");
        }
    }
    else die_errno("Wrong type of msg in to_all_or_friends!");
}

void to_one(int cl_from, int cl_to, char *string){ // supposing cl_to is on the list
    // as given in the instructions
    if(!string) die_errno("empty string in to_one()");
    
    char *res_string = malloc(MAX_MSG_SIZE);
    strcpy(res_string, "Client with ID = ");

    char *msqid_str = malloc(MAX_ID_LENGTH);
    sprintf(msqid_str, "%d", cl_from);

    strcat(res_string, msqid_str);
    strcat(res_string, " wrote to client with ID = ");
    sprintf(msqid_str, "%d", cl_to);
    strcat(res_string, msqid_str);
    strcat(res_string, ": \"");
    strcat(res_string, string);
    strcat(res_string, "\" on ");
    strcat(res_string, get_date_time());
    printf("FINAL MESSAGE: %s\n", res_string);

    send_msg(cl_to, TO_ONE, res_string, "msgsnd, to_one()");
}

char *extract_ID_from_str(char *string){
    char *str_copy = malloc(MAX_MSG_SIZE * sizeof(char));
    strcpy(str_copy, string);
    char *ID = strtok_r(str_copy, " ", &str_copy);
    if(ID == NULL) die_errno("extracting ID... ID is NULL!");
    return ID;
}

void init_array_and_vars(){
    friendsIDs = calloc(MAX_CL_COUNT * (1 + MAX_ID_LENGTH), sizeof(char)); // counting the spaces between IDs
    clientCount = 0;
    friends_count = 0;
    for(int i = 0; i < MAX_CL_COUNT; i++)
        clients[i].clientID = clients[i].pid = -1;
}

void SIGINThandler(int signum){
    msg *stop_msg;
    printf("SERVER: Received SIGINT. Quiting...");
    // sending to all clients SIGINT && waiting for all clients to send STOP
    for(int i = 0; i < clientCount; i++){
        kill(clients[i].pid, SIGINT);
        sleep(1);
        if(clients[i].clientID != -1)
            if((stop_msg = receive_msg(clients[i].clientID, STOP, 0)) == NULL){
                printf("client ID = %d\n", clients[i].clientID);
                die_errno("didn't receive STOP message from client");
            }
        rm_client_queue(clients[i].clientID);
    }
    exit(0);
}


///
///
// WORKING ON IT
///
///

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
    clients[clientCount].clientID = cl_msqid;
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
    clientCount++;
    printf("MSG SENT TO CLIENT\n");
}

void decode_message(msg *message){
    switch (message->mtype){
        case ECHO:
            echo(message->msqid, message->mtext); break;
        case LIST:
            list(); break;
        case FRIENDS:
            friends(message->mtext); break;
        case ADD:
            add(message->mtext); break;
        case DEL:
            del(message->mtext); break;
        case TO_ALL:
            to_all_or_friends(TO_ALL, message->msqid, message->mtext); break;
        case TO_FRIENDS:
            to_all_or_friends(TO_FRIENDS, message->msqid, message->mtext); break;
        case TO_ONE: // ID is the first elem of the string
            to_one((int) strtol(extract_ID_from_str(message->mtext), NULL, 10), 
                message->msqid, message->mtext); break;
        case STOP:
            stop(message->msqid);
            break;
        default:
            break;
    }
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
    printf("\n\nALL CLIENTS AFTER LOGIN:\n");
    for(int i = 0; i < MAX_CL_COUNT; i++)
        if(clients[i].clientID != -1)
            printf("i: %d, clientID: %d\n", i, clients[i].clientID);

    printf("\n\n");
    // --- changing it
    msg *rcvd_msg;
    int clientID;
    sleep(2);
    // add("222 234");
    // to_all_or_friends(TO_FRIENDS, 100, "ALA ma KOTA");
    
    ///////////////////////////////////////////////////////////////
    while(1){ // serving clients according to clientsInd
        // printf("\nFirst things first\n");
        for(int i = 1; i <= 3; i++) // types: STOP, LIST, FRIENDS are served first
            for(int j = 0; j < MAX_CL_COUNT; j++){
                clientID = clients[j].clientID;
                // printf("j: %d, ClientID: %d\n", j, clientID);
                if(clientID == -1) continue;
                while((rcvd_msg = receive_msg(clientID, i, IPC_NOWAIT)) != NULL){
                    printf("RECEIVED MESSAGE: %s, type: %d, from %d\n", rcvd_msg->mtext, (int) rcvd_msg->mtype, clientID);
                    decode_message(rcvd_msg);
                }
            }
    
        // then the remaining requests are served
        sleep(1);
        printf("\nThen the rest\n");
        // serving client whose id == clients[clientsInd].clientID
            // receiving ALL messages of ANY TYPE because STOP message cannot come before INIT
            // It would have been received if it had come.
        clientID = clients[clientsInd].clientID;
        if(clientID != -1){
            printf("Messages from client: %d\n", clientID);
            while((rcvd_msg = receive_msg(clientID, 0, IPC_NOWAIT)) != NULL){
                printf("RECEIVED MESSAGE: %s, type: %d, from %d\n", rcvd_msg->mtext, (int) rcvd_msg->mtype, clientID);
                sleep(1);
                decode_message(rcvd_msg);
            }
        }
        clientsInd++; // serving next client;
        clientsInd %= MAX_CL_COUNT+1;
    }
    return 0;
}

// works :)
// - with all: echo(clients[0].clientID, "heeeeej");
// - with one client (for sure): list();
// - friends("234 123 456 777"); <- but they have to be connected
// - add("222 234");
// - del("123");
// - to_all_or_friends(TO_FRIENDS, 100, "ALA ma KOTA");
// to_one(100, 200, "ALA NIE ma PSAA haha!");