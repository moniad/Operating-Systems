
// DOKUMENTACJA:
// loaders_manager.c tworzy mi tylko procesy
// mam K miejsc w belt[]. Do belt[0] dokładam paczkę i odbieram paczkę spod ostatniego indeksu 
// są 3 semafory:
// - jeden - binarny - tylko dla loaderów do wkładania paczek na taśmę
// - drugi - binarny - dla truckera i loaderów, aby w jednej chwili była wykonywana
//      tylko jedna operacja na taśmie (ściągnięcie lub włożenie paczki)
// - trzeci - wielowartościowy - wspólny dla truckera i loaderów, aby trucker ściągnął
//      przynajmniej tyle paczek, ile trzeba, aby jakiś loader mógł dołożyć swoją paczkę

// EXAMPLE INPUT:
// ./trucker.o 2 4 30
// ./loaders_manager.o 2
// 15 2 <- trzeba tu wpisać -1, jeśli ma nie być cyklu
// 14 3


#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <wait.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>


#define PROJECT_ID 7
#define MAX_PCKGS_NO 100
#define END_LINE_SEMAPHORE 0 // trucker knows there's a package to get off the belt
#define TRUCK_SEMAPHORE 1 // semaphore for trucker capacity, not to exceed it
#define START_LINE_SEMAPHORE 2 // one loader can load at a time
#define DATE_LENGTH 30


typedef struct Truck{
    int maxBoxCount;
    int currentWeight;
    int currentBoxNumber;
} Truck;

typedef struct package {
    int weight;
    pid_t workerID;
    struct timeval loadTime;
} package;

typedef struct Belt {
    int maxWeight;
    int maxBoxes;
    int currentWeight;
    int currentBoxes;
    int currentBoxInLine;
    int truckEnded;
    package line[MAX_PCKGS_NO];
} Belt;

sem_t* semaphores[3];
char datetime[DATE_LENGTH];
const char format[] = "%d-%m-%Y %H:%M:%S";

void die_errno(char *msg){
    perror(msg);
    exit(1);
}

char *get_date_time(){ // datetime is statically allocated, but it's not a problem
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if(strftime(datetime, DATE_LENGTH, format, &tm) < 0) die_errno("strftime");
    return datetime;
}

void releaseSemaphore(int semaphore) {
    sem_post(semaphores[semaphore]);
}

void takeSemaphore(int semaphore) {
    sem_wait(semaphores[semaphore]);
}

int tryToTakeSemaphore(int semaphore) {
    if (sem_trywait(semaphores[semaphore]) >= 0)
        return 1;
    return 0;
}

void putBox(Belt *belt, package package) {
    sleep(rand() % 2);
    int sent = 0;

    while (!sent) {
        // sleep(rand() % 1);
        if (belt->truckEnded == 1 && belt->currentWeight == 0)
            exit(0);
        takeSemaphore(START_LINE_SEMAPHORE);
        if (belt->maxWeight >= package.weight + belt->currentWeight &&
            belt->currentBoxes + 1 <= belt->maxBoxes) {
            if(tryToTakeSemaphore(TRUCK_SEMAPHORE) == 1){
                gettimeofday(&package.loadTime, NULL);
                belt->line[belt->currentBoxInLine++] = package;
                belt->currentBoxInLine %= MAX_PCKGS_NO;
                belt->currentWeight += package.weight;
                belt->currentBoxes++;
                sent = 1;
                releaseSemaphore(END_LINE_SEMAPHORE);
            } else {
                printf("PID: %d. Ciężarówka jest pełna, czekam.\n", getpid());
            }
        }
        releaseSemaphore(START_LINE_SEMAPHORE);
    }
    
    printf("Proces PID=%d położył paczkę o MASIE=%d | CZAS:%ld | TIME_STAMP: %s | wolna waga:%d | wolne miejsca:%d\n",
           package.workerID, package.weight, package.loadTime.tv_usec, get_date_time(), belt->maxWeight - belt->currentWeight,
           belt->maxBoxes - belt->currentBoxes);
    if (belt->truckEnded == 1 && belt->currentWeight == 0)
        exit(0);
}
#endif //COMMON_H