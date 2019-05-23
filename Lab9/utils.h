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


// typedef struct Car {
//     int threadNumber;
//     int *peopleTIDs;
//     // int curPassengersCount;
// } Car;

typedef struct RollerCoaster {
    int passengerCount;
    int carCount;
    int carCapacity;
    int ridesCount;
    // Car *currentCarsState;
} RollerCoaster;

struct timespec spec;

void die_errno(char *message) {
    printf("ERROR: %s\n", message);
    exit(1);
}

void printTimeStamp()
{
    long ms; // milliseconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    ms = spec.tv_sec * 1000 + spec.tv_nsec / 1000;
    printf("Timestamp: %" PRIdMAX" miliseconds. ", ms);
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