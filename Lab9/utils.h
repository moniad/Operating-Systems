#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

typedef struct Car {
    int ID;
    int passengersCount;
    int *passengersIDs;
} Car;

typedef struct RollerCoaster {
    int totalPassengerCount;
    int carCount;
    int carCapacity;
    int leftRidesCount;
    Car *cars;
} RollerCoaster;

struct timespec spec;

pthread_t *psgTIDs, *carTIDs; // passenger and car TIDs
int carTIDsOffset; // car IDs start from this offset
int *passengerConsequentIDs; // storing there passenger IDs
int *carConsequentIDs;
int consequentIDsSize;

// current data
int curCarInd; // which car is currently at the platform
int canEnterCar; // for passenger
int canPressStart;
int startIndex; // index in table of curCarPassengers telling which passenger will press start  
int passengersKnowingNewStartIndex;
int leftCarsCount; // if == 0, then all passengers should die

// mutexes and condition variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPassengerEnterCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condOpenDoor = PTHREAD_COND_INITIALIZER;
pthread_cond_t condFullCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condEmptyCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condFinishedRide = PTHREAD_COND_INITIALIZER;
pthread_cond_t condCanArriveNextCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condPressedStart = PTHREAD_COND_INITIALIZER; // to wait until START button is pressed
pthread_cond_t condCanPressStart = PTHREAD_COND_INITIALIZER; // to tell the passenger to press it
pthread_cond_t condKnownNewStartIndex = PTHREAD_COND_INITIALIZER; // to randomly find which passenger will press start
pthread_cond_t condAllReceivedNewStartIndex = PTHREAD_COND_INITIALIZER;

void doTheCleanUp() {
    free(passengerConsequentIDs);
    free(carConsequentIDs);
    free(psgTIDs);
    free(carTIDs);
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condEmptyCar);
    pthread_cond_destroy(&condFinishedRide);
    pthread_cond_destroy(&condFullCar);
    pthread_cond_destroy(&condCanArriveNextCar);
    pthread_cond_destroy(&condPassengerEnterCar);
    pthread_cond_destroy(&condOpenDoor);
    pthread_cond_destroy(&condPressedStart);
    pthread_cond_destroy(&condCanPressStart);
    pthread_cond_destroy(&condKnownNewStartIndex);
    pthread_cond_destroy(&condAllReceivedNewStartIndex);
}

void printCurrentCarsState(RollerCoaster RC){
    printf("\nPrinting current cars state:\n");
    for(int i = 0; i < RC.carCount; i++){
        printf("ID: %d, ", RC.cars[i].ID);
        printf("Passenger count: %d\n", RC.cars[i].passengersCount);
        printf("Passenger IDs:\n");

        for(int j = 0; j < RC.cars[i].passengersCount; j++)
            printf("%d ", RC.cars[i].passengersIDs[j]);
        
        printf("\n\n");
    }
}

void die_errno(char *message) {
    printf("ERROR: %s\n", message);
    exit(1);
}

void printTimeStamp() {
    long ms; // milliseconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    ms = spec.tv_sec * 1000 + spec.tv_nsec / 1000;
    printf("Timestamp: %" PRIdMAX" ms. ", ms);
}

// car messages
void printClosingDoor(pthread_t tid){
    printTimeStamp();
    printf("CAR: %d: Closing door... Closed.\n", (int) tid);
}

void printStartedRide(pthread_t tid){
    printTimeStamp();
    printf("CAR: %d: Ride started\n", (int) tid);
}

void printFinishedRide(pthread_t tid){
    printTimeStamp();
    printf("CAR: %d: Ride finished\n", (int) tid);
    printf("===========================================================================\n");
}

void printOpeningDoor(pthread_t tid){
    printTimeStamp();
    printf("CAR: %d: Opening door... Open.\n", (int) tid);
}

// common
void printFinishedThread(pthread_t tid, char *type){
    printTimeStamp();
    printf("Thread %s = %d finished.\n", type, (int) tid);
}

// passenger messages
void printEnteringCar(pthread_t tid, int curPsgCount){
    printTimeStamp();
    printf("PASSENGER %d: Entering the car. Current passenger count: %d\n", (int) tid, curPsgCount);
}

void printLeavingCar(pthread_t tid, int curPsgCount){
    printTimeStamp();
    printf("PASSENGER %d: Leaving car. Current passenger count: %d\n", (int) tid, curPsgCount);
}

void printPressedStart(pthread_t tid){
    printTimeStamp();
    printf("PASSENGER %d: Pressed start.\n", (int) tid);
}

#endif