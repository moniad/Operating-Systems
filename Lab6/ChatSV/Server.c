/*Napisz prosty chat typu klient-serwer, w którym komunikacja zrealizowana jest za 
pomocą kolejek komunikatów - jedna, na zlecenia klientów dla serwera, druga, prywatna,
na odpowiedzi.

Wysyłane zlecenia klientow do serwera mają zawierać 
1) rodzaj zlecenia jako rodzaj komunikatu
2) informację od którego klienta zostały wysłane (ID klienta).

W odpowiedzi rodzajem komunikatu ma być 
1) informacja identyfikująca czekającego na nią klienta.

 Po otrzymaniu identyfikatora, 
klient rozpoczyna wysyłanie zleceń do serwera (w pętli), zlecenia są czytane ze 
standardowego wyjścia w postaci typ_komunikatu albo z pliku tekstowego w którym
 w każdej linii znajduje się jeden komunikat (napisanie po stronie klienta READ 
 plik zamiast typu komunikatu). Przygotuj pliki z dużą liczbą zleceń, aby można 
 było przetestować działanie zleceń i priorytetów.
*/

#include <stdio.h> // perror()
#include <sys/types.h> // msgget(), ftok()
#include <sys/ipc.h> // msgget(), ftok()
#include <sys/msg.h> // msgget()
#include <stdlib.h> // system()
#include <string.h>
#include <signal.h> // sigaction(), sigprocmask()
#include <unistd.h> // sleep()
#include "Server.h"

// key_t clients[MAX_CL_COUNT]; // client keys -- maybe IDs instead???
int clientsID[MAX_CL_COUNT];
/*
clients
change login_client()
SIGINThandler() - to do - receive stop from all clients

Zlecenia powinny być obsługiwane zgodnie z priorytetami, 
najwyższy priorytet ma STOP, potem LIST oraz FRIENDS i reszta. 

Serwer może wysłać do klientów komunikaty:
inicjujący pracę klienta (kolejka główna serwera)
wysyłający odpowiedzi do klientów (kolejki klientów)
informujący klientów o zakończeniu pracy serwera - po wysłaniu takiego sygnału i odebraniu wiadomości STOP od wszystkich klientów serwer usuwa swoją kolejkę i kończy pracę. (kolejki klientów)
Należy obsłużyć przerwanie działania serwera lub klienta za pomocą CTRL+C. Po stronie klienta obsługa tego sygnału jest równoważna z wysłaniem komunikatu STOP.


*/
int flags = IPC_CREAT | 0666;
int serv_msqid; // id of server queue for clients to send their messages to server
int clientCount;
int command = IPC_RMID; // for atexit()
struct sigaction act;

typedef struct msg{
    long mtype;
    char mtext[MAX_MSG_SIZE];
} msg;

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

struct msg receive_msg(int msqid, int type){
    msg rcvd_init_msg;
    if(msgrcv(msqid, &rcvd_init_msg, MAX_MSG_SIZE, type, 0) < 0){
        die_errno("server msgrcv");
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
    msg rcvd_init_msg = receive_msg(serv_msqid, INIT);
    printf("RECEIVED: %s\n", rcvd_init_msg.mtext);
    key_t cl_key = (int) strtol(rcvd_init_msg.mtext, NULL, 10);
    // clients[++/nothingclientCount] = (int) strtol(rcvd_init_msg.mtext, NULL, 10);
   
    //-------------------
    // answer 
    // client using their queue. we know its key because it was sent in the init msg
    if((cl_msqid = msgget(cl_key, flags)) < 0){
        die_errno("msget, answering client");
    }
    printf("CLIENT ID: %d\n", cl_msqid);
    clientsID[++clientCount] = cl_msqid;

    msg answer;
    answer.mtype = ANS;
    sprintf(answer.mtext, "%d", clientCount);
        // send answer
    if(msgsnd(cl_msqid, &answer, strlen(answer.mtext)+1, IPC_NOWAIT) < 0){
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
    // wait for all clients to send STOP

    for(int i = 0; i < clientCount; i++)
        receive_msg(clientsID[i], END);
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

int main(void){
   
   set_signal_handling();

    clientCount = -1;
    // give the process read & write permission
    if((serv_msqid = msgget(SERV_KEY, flags)) < 0){
        die_errno("msget");
    }
    if(atexit(rm_queue) < 0){
        die_errno("atexit");
    }
    login_client(); // may need changes
    sleep(4);
    return 0;
}

/*  receive stop from Client. LOGGING IN A CLIENT SHOULD BE CHANGED (WE CAN FIND
 PLACES WITH -1 THERE? ?? ? IS IT OK???) ID probably ok, but I need to rmv a QUEUE!
    
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