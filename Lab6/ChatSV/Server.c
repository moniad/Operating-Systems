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
#include "Server.h"

key_t clients[MAX_CL_COUNT]; // client keys -- maybe IDs instead???

/*
clients
change login_client()
work on atexit


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
    // msg rcvd_init_msg = receive_msg(serv_msqid, INIT);
    msg rcvd_init_msg;
    if(msgrcv(serv_msqid, &rcvd_init_msg, MAX_MSG_SIZE, 1, 0) < 0){
        die_errno("server msgrcv");
    }
    ++clientCount;
    printf("RECEIVED: %s\n", rcvd_init_msg.mtext);

    //-------------------
    // answer 
    // client using their queue. we know its key because it was sent in the init msg
    if((cl_msqid = msgget((int) strtol(rcvd_init_msg.mtext, NULL, 10), flags)) < 0){
        die_errno("msget, answering client");
    }
    printf("CLIENT ID: %d\n", cl_msqid);

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

int main(void){
    clientCount = -1;
    // give the process read & write permission
    if((serv_msqid = msgget(SERV_KEY, flags)) < 0){
        die_errno("msget");
    }
    if(atexit(rm_queue) < 0){
        die_errno("atexit");
    }
    login_client(); // may need changes

    return 0;
}