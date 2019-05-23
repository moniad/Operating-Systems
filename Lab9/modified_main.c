/* not including pressing START button yet 
    and car sequence 
    */

#include "utils.h"

RollerCoaster RC;
pthread_t *carTIDs, *psgTIDs;
int carTIDsOffset;
int consequentIDsSize;
int *consequentIDs;

int curCarID;
int curPassengerCount;
int canEnterCar;
int canLeaveCar;
int canArriveNextCar;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPassengerEnterCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condPassengerLeaveCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condFullCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condEmptyCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condCanArriveNextCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condAllCarsFull = PTHREAD_COND_INITIALIZER;

void ride(int thread_no) {
    printStartedRide(thread_no);
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = 1e6 * (rand() % 10);
    printf("nsec: %ld\n", (long) spec.tv_nsec);
    if(nanosleep(&spec, NULL) < 0) die_errno("ride: nanosleep");
    printFinishedRide(thread_no);
}

void doSthBeforeExitInPassenger(int thread_no) {
    pthread_cond_broadcast(&condFullCar);
    pthread_mutex_unlock(&mutex);
    printFinishedThread(thread_no, "PASSENGER");
}

void* passenger(void *thread_num) {
    int thread_no = *(int *) thread_num;
    // printf("\n I'm a passenger: %d \n", thread_no);

    pthread_cond_signal(&condCanArriveNextCar);
    canArriveNextCar = 1;
    while(RC.ridesCount >= 0){

        pthread_mutex_lock(&mutex);

        // wait for people to get off the car        
        if(curPassengerCount > 0) {
            while(!canLeaveCar){
                pthread_cond_wait(&condPassengerLeaveCar, &mutex);
            }
            curPassengerCount--;
            printLeavingCar(thread_no, curPassengerCount);
        }

        if(curPassengerCount == 0) {
            pthread_cond_signal(&condEmptyCar);
        }
        
        pthread_mutex_unlock(&mutex);

        
        pthread_mutex_lock(&mutex);

        // wait until passenger can enter the car
        while(!canEnterCar){
            pthread_cond_wait(&condPassengerEnterCar, &mutex);
            if(curPassengerCount >= RC.carCapacity) continue;
            // printf("curPassengerCount : %d\n", curPassengerCount);
        }

        curPassengerCount++;
        printEnteringCar(thread_no, curPassengerCount);

        if(curPassengerCount == RC.carCapacity) {
            // signal curCar is full and move to next car
            pthread_cond_broadcast(&condFullCar);
        }

        pthread_mutex_unlock(&mutex);
    

    doSthBeforeExitInPassenger(thread_no);
    return NULL;
}

void doSthBeforeExitInCar(int thread_no) {
    // to unlock waiting passenger threads
    pthread_cond_broadcast(&condPassengerEnterCar);

    printFinishedThread(thread_no, "CAR");
}

void* car(void *thread_num) {
    int thread_no = *(int *) thread_num;
    // printf("\n I'm a car: %d \n", thread_no);

    while(RC.ridesCount >= 0){

        pthread_mutex_lock(&mutex);
        
        /*
                poczekaj, aż będzie twoja kolej (i-ty index w tablicy aut)
            */        

        while(!canArriveNextCar) {
            pthread_cond_wait(&condCanArriveNextCar, &mutex);
            if(thread_no != curCarID) continue;
        }
        canArriveNextCar = 0;
        
        printOpeningDoor(thread_no);
        canEnterCar = 0;

        // let people leave the car
        if(curPassengerCount > 0) {
            canLeaveCar = 1;
            for(int i = 0; i < RC.carCapacity; i++)
                pthread_cond_signal(&condPassengerLeaveCar);
            // printf("TELLING THEM TO LEAVE....\n");
        }

        pthread_cond_wait(&condEmptyCar, &mutex);
        canLeaveCar = 0;
        
        // let people enter the car
        canEnterCar = 1;
        for(int i = 0; i < RC.carCapacity; i++){
            pthread_cond_signal(&condPassengerEnterCar);
        }

        if(curPassengerCount < RC.carCapacity)
            pthread_cond_wait(&condFullCar, &mutex);
        
        printClosingDoor(thread_no);

        if(thread_no == carTIDsOffset + RC.carCount - 1) { // if all are full
            /*
                poczekaj, aż n wagonów się zapełni
            */                
            RC.ridesCount--;
            ride(thread_no);
        }

        // let next car arrive
        canArriveNextCar = 1;
        curCarID = (curCarID + 1 - carTIDsOffset) % RC.carCount + carTIDsOffset;
        pthread_cond_signal(&condCanArriveNextCar);

        pthread_mutex_unlock(&mutex);

    }

    doSthBeforeExitInCar(thread_no);
    return NULL;
}


void parse_input(int argc, char **argv) {
    if (argc != 5) die_errno("Pass: \n- passengerCount \n- carCount \n- carCapacity \n- ridesCount\n");

    RC.passengerCount = atoi(argv[1]);
    RC.carCount = atoi(argv[2]);
    RC.carCapacity = atoi(argv[3]);
    RC.ridesCount = atoi(argv[4]) - 1;

    printf("passengerCount: %d, ", RC.passengerCount);
    printf("carCount: %d, ", RC.carCount);
    printf("carCapacity: %d, ", RC.carCapacity);
    printf("ridesCount: %d\n", RC.ridesCount);

    carTIDs = malloc(RC.carCount * sizeof(pthread_t));

    consequentIDsSize = RC.passengerCount > RC.carCount ? RC.passengerCount : RC.carCount;
    consequentIDs = malloc(consequentIDsSize * sizeof(int));
    for (int i = 0; i < consequentIDsSize; i++)
        consequentIDs[i] = i;
    
    carTIDsOffset = RC.passengerCount + 10200;

    psgTIDs = malloc(RC.passengerCount * sizeof(pthread_t));

    curCarID = carTIDsOffset;
    curPassengerCount = 0;
    canEnterCar = 0;
    canLeaveCar = 0;
}

void createPassengerThreads() {
    for (int i = 0; i < RC.passengerCount; i++)
        if(pthread_create(&psgTIDs[i], NULL, &passenger, &consequentIDs[i]) < 0) 
            die_errno("creating passenger threads");
}

void createCarThreads() {
    // increment my threads IDs 
    for (int i = 0; i < RC.carCount; i++)
        consequentIDs[i] += carTIDsOffset;
    
    for (int i = 0; i < RC.carCount; i++)
        if(pthread_create(&carTIDs[i], NULL, &car, &consequentIDs[i]) < 0) 
            die_errno("creating car threads");
}

void waitForPassengers(){
    for (int i = 0; i < RC.passengerCount; i++)
        pthread_join(psgTIDs[i], NULL);
}

void waitForCars(){
    for (int i = 0; i < RC.carCount; i++)
        pthread_join(carTIDs[i], NULL);
}

void waitUntilAllThreadsFinish(){
    waitForPassengers();
    waitForCars();
}

int main(int argc, char **argv)
{
    parse_input(argc, argv);

    createPassengerThreads();

    createCarThreads();

    waitUntilAllThreadsFinish();


    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condAllCarsFull);
    pthread_cond_destroy(&condEmptyCar);
    pthread_cond_destroy(&condFullCar);
    pthread_cond_destroy(&condCanArriveNextCar);
    pthread_cond_destroy(&condPassengerEnterCar);
    pthread_cond_destroy(&condPassengerLeaveCar);

    return 0;
}