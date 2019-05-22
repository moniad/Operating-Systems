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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPassengerEnterCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condPassengerLeaveCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condFullCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condAllCarsFull = PTHREAD_COND_INITIALIZER;
pthread_cond_t condEmptyCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNoMoreRides = PTHREAD_COND_INITIALIZER;

void ride(int thread_no) {
    printStartedRide(thread_no);
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = 1e6 * (rand() % 10);
    printf("nsec: %ld\n", (long) spec.tv_nsec);
    if(nanosleep(&spec, NULL) < 0) die_errno("ride: nanosleep");
    printFinishedRide(thread_no);
}

void* passenger(void *thread_num) {
    int thread_no = *(int *) thread_num;
    // printf("\n I'm a passenger: %d \n", thread_no);

    while(RC.ridesCount >= 0){

        pthread_mutex_lock(&mutex);

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

        if(RC.ridesCount > 0){

            pthread_mutex_lock(&mutex);

            while(!canEnterCar){
                pthread_cond_wait(&condPassengerEnterCar, &mutex);
               
                if(RC.ridesCount == 0) {
                    pthread_mutex_unlock(&mutex);
                    printFinishedThread(thread_no, "PASSENGER");
                    return NULL;
                }
                if(curPassengerCount >= RC.carCapacity) continue;
                // printf("curPassengerCount : %d\n", curPassengerCount);
            }

            curPassengerCount++;
            
            printEnteringCar(thread_no, curPassengerCount);

            if(curPassengerCount == RC.carCapacity)
                pthread_cond_broadcast(&condFullCar);

            pthread_mutex_unlock(&mutex);
        }
        else {
            pthread_mutex_unlock(&mutex);
            break;
        }
        // printf("rides count: %d\n\n", RC.ridesCount);
    }

    printFinishedThread(thread_no, "PASSENGER");
    return NULL;
}

void* car(void *thread_num) {
    int thread_no = *(int *) thread_num;
    // printf("\n I'm a car: %d \n", thread_no);

    while(RC.ridesCount >= 0){

        pthread_mutex_lock(&mutex);
        
        printOpeningDoor(thread_no);

        if(curPassengerCount != 0) {
            canLeaveCar = 1;
            for(int i = 0; i < RC.carCapacity; i++)
                pthread_cond_signal(&condPassengerLeaveCar);
            // printf("TELLING THEM TO LEAVE....\n");
        }

        while(curPassengerCount != 0)
            pthread_cond_wait(&condEmptyCar, &mutex);

        if(RC.ridesCount > 0) {
            if(curPassengerCount == 0){
                canLeaveCar = 0;
                canEnterCar = 1;
                for(int i = 0; i < RC.carCapacity; i++){
                    pthread_cond_signal(&condPassengerEnterCar);
                }
            }

            /*
                poczekaj, aż będzie twoja kolej (i-ty index w tablicy aut)
            */

            while(curPassengerCount < RC.carCapacity){
                pthread_cond_wait(&condFullCar, &mutex);
            }
            canEnterCar = 0;

            /*
                poczekaj, aż n wagonów się zapełni
            */

            printClosingDoor(thread_no);
            RC.ridesCount--;
            ride(thread_no);
            pthread_mutex_unlock(&mutex);
        }
        else {
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    // to unlock waiting passenger threads
    pthread_cond_broadcast(&condPassengerEnterCar);

    printFinishedThread(thread_no, "CAR");
    return NULL;
}


void parse_input(int argc, char **argv) {
    if (argc != 5) die_errno("Pass: \n- passengerCount \n- carCount \n- carCapacity \n- ridesCount\n");

    RC.passengerCount = atoi(argv[1]);
    RC.carCount = atoi(argv[2]);
    RC.carCapacity = atoi(argv[3]);
    RC.ridesCount = atoi(argv[4]);

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

    curCarID = 0;
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
    pthread_cond_destroy(&condNoMoreRides);
    pthread_cond_destroy(&condPassengerEnterCar);
    pthread_cond_destroy(&condPassengerLeaveCar);

    return 0;
}