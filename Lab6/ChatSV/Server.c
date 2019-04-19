/*Napisz prosty chat typu klient-serwer, w którym komunikacja zrealizowana jest za 
pomocą kolejek komunikatów - jedna, na zlecenia klientów dla serwera, druga, prywatna,
 na odpowiedzi.

Wysyłane zlecenia klientow do serwera mają zawierać 
1) rodzaj zlecenia jako rodzaj komunikatu
2) informację od którego klienta zostały wysłane (ID klienta).

W odpowiedzi rodzajem komunikatu ma być 
1) informacja identyfikująca czekającego na nią klienta.

Klient bezpośrednio po uruchomieniu tworzy kolejkę z unikalnym kluczem IPC  i wysyła 
jej klucz komunikatem do serwera (komunikat INIT). Po otrzymaniu takiego komunikatu, 
serwer otwiera kolejkę klienta, przydziela klientowi identyfikator (np. numer 
w kolejności zgłoszeń) i odsyła ten identyfikator do klienta (komunikacja w kierunku 
serwer->klient odbywa się za pomocą kolejki klienta). Po otrzymaniu identyfikatora, 
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

int flags = IPC_CREAT | 0666;
// server key = 112

/* TYPES OF MESSAGES:
999 - ANSWER
*/

// do it differently, IT MIGHT WORK ONLY FOR ONE PROCESS ???

// first receiving message of type 1 - init msg

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

// void answer(); // ogarnąć: w odpowiedzi rodzajem komunikatu 
// ma być informacja identyfikująca czekającego na nią klienta.
/////////////// a table of client keys??? NA RAZIE DLA JEDNEGO

int main(void){
    // key_t serv_key = ;
    int serv_msqid; // id of server queue for clients to send their messages to server
    int cl_msqid;
    ///////////// probably there should be a table of client keys

    int msgCount = -1;
    // give the process has read & write permission
    if((serv_msqid = msgget(SERV_KEY, flags)) < 0){
        die_errno("msget");
    }
    // receive message
    // msg rcvd_init_msg = receive_msg(serv_msqid, INIT);
    msg rcvd_init_msg;
    if(msgrcv(serv_msqid, &rcvd_init_msg, MAX_MSG_SIZE, 1, 0) < 0){
        die_errno("server msgrcv");
    }
    ++msgCount;
    printf("RECEIVED: %s\n", rcvd_init_msg.mtext);

    // //-------------------
    // // answer 
    // // client using their queue. we know its key because it was sent in the init msg
    // if((cl_msqid = msgget(strtol(rcvd_init_msg.mtext, NULL, 10), flags)) < 0){
    //     die_errno("msget, answering client");
    // }
    // printf("KEY: %s", rcvd_init_msg.mtext);
    // msg answer;
    // answer.mtype = 999;
    // sprintf(answer.mtext, "%d", msgCount);
    // answer.key = rcvd_init_msg.key;
    //     // send answer
    // if(msgsnd(answer.key, &answer, strlen(answer.mtext)+1, IPC_NOWAIT) < 0){
    //    die_errno("msgsnd, answering client");
    // }


    return 0;
}