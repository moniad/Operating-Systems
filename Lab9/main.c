/* not including pressing START button yet 
    and car sequence 
    */

#include "utils.h"

RollerCoaster RC;
pthread_t *psgTIDs, *carTIDs;
int carTIDsOffset;
int consequentIDsSize;
int *consequentIDs;

int curCarID;
int curPassengerCount;
int canEnterCar;
int isRideFinished;

// int leftCarsCount;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPassengerEnterCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condFinishedRide = PTHREAD_COND_INITIALIZER;
pthread_cond_t condFullCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condEmptyCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condCanArriveNextCar = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNewCarToGetOffArrived = PTHREAD_COND_INITIALIZER;

void ride(int thread_no) {    
    printStartedRide(thread_no);
    
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = 1e6 * (rand() % 10);
    printf("RIDING for %ld nsec.\n", (long) spec.tv_nsec);
    if(nanosleep(&spec, NULL) < 0) die_errno("ride: nanosleep");

    printFinishedRide(thread_no);

    if(thread_no == RC.carCount + carTIDsOffset - 1)
        RC.leftRidesCount--;
    isRideFinished = 1;

    // let people leave the car

    printf("I should wait here until all passengers get off!\n");

    printOpeningDoor(thread_no);

    // if(
        // RC.leftRidesCount == 0) {
        // leftCarsCount = 0;
    // }

    isRideFinished = 0;
    pthread_cond_broadcast(&condFinishedRide);
    pthread_cond_wait(&condEmptyCar, &mutex);

    printf("CAR %d: Empty car\n", thread_no);
}

void doSthBeforeExitInPassenger(int thread_no) {
    // pthread_mutex_lock(&mutex);
    printFinishedThread(thread_no, "PASSENGER");
    pthread_mutex_unlock(&mutex);
}

void addPassengerToCar(int thread_no) {
    curPassengerCount++;
    printEnteringCar(thread_no, curPassengerCount);
}

void rmvPassengerFromCar(int thread_no) {
    curPassengerCount--;
    printLeavingCar(thread_no, curPassengerCount);
}

void* passenger(void *thread_num) {
    int thread_no = *(int *) thread_num;
    // printf("\n I'm a passenger: %d \n", thread_no);
 
    pthread_cond_broadcast(&condEmptyCar);

    // int i = 5;
    while(1){ // leftCarsCount

        pthread_mutex_lock(&mutex);

        // wait until passenger can enter the car
        while(!canEnterCar){
            printf("PSSNGEEER: %d Waiting until can enter car\n", thread_no);
            pthread_cond_wait(&condPassengerEnterCar, &mutex);

            // printf("PASSNGRRR %d: Autek jest tyle: %d\n", thread_no, leftCarsCount);
            if(RC.leftRidesCount == 0) break; // leftCarsCount

            if(curPassengerCount >= RC.carCapacity) {
                // pthread_cond_broadcast(&condFullCar);
                continue;
            }
            printf("PSNGEEEERRRRRRRR: %d: Will STOP Waiting???\n", thread_no);
            // printf("curPassengerCount : %d\n", curPassengerCount);
        }

        if(RC.leftRidesCount == 0) { //leftCarsCount == 0) {
            // pthread_mutex_unlock(&mutex);
            break;
        }

        addPassengerToCar(thread_no);

        if(curPassengerCount == RC.carCapacity) {
            // signal that curCar is full
            pthread_cond_broadcast(&condFullCar);
        }

        // wait for people to get off the car        
        if(curPassengerCount > 0) {
            if(!isRideFinished){ // while
                printf("PSNGGRRR: %d: Waiting until RIDE is FINISHED\n", thread_no);
                pthread_cond_wait(&condFinishedRide, &mutex);
            }

            rmvPassengerFromCar(thread_no);
            if(curPassengerCount == 0) {
                printf("PSSNGEEER: %d Signaling empty car\n", thread_no);
                pthread_cond_signal(&condEmptyCar);
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    doSthBeforeExitInPassenger(thread_no);
    return NULL;
}

void doSthBeforeExitInCar(int thread_no) {
    // --leftCarsCount;

// added this lines
    pthread_cond_broadcast(&condCanArriveNextCar);
    canEnterCar = 1;
//

    pthread_cond_broadcast(&condPassengerEnterCar);

    printFinishedThread(thread_no, "CAR");

    pthread_mutex_unlock(&mutex);
}

void* car(void *thread_num) {
    int thread_no = *(int *) thread_num;
    // printf("\n I'm a car: %d \n", thread_no);
    
    pthread_cond_broadcast(&condCanArriveNextCar);

    while(1) { // RC.leftRidesCount >= 0){

        pthread_mutex_lock(&mutex);

        // wait for your turn
        while(thread_no != curCarID) {
            pthread_cond_wait(&condCanArriveNextCar, &mutex);
            if(RC.leftRidesCount == 0) break;
        }

        if(RC.leftRidesCount == 0 /*|| leftCarsCount == 0 */) {
            printf("CAR: %d: Who has ended their trip?????\n", thread_no);
            break;
        }

        printOpeningDoor(thread_no);

        // let people enter the car
        canEnterCar = 1;
        for(int i = 0; i < RC.carCapacity; i++){
            pthread_cond_signal(&condPassengerEnterCar);
        }
        printf("TID: %d: now waiting for full car!\n", thread_no);
        // if(curPassengerCount < RC.carCapacity)
        pthread_cond_wait(&condFullCar, &mutex);
        
        canEnterCar = 0;
        printClosingDoor(thread_no);              
        ride(thread_no);

        // let next car arrive
        curCarID = (curCarID + 1 - carTIDsOffset) % RC.carCount + carTIDsOffset;
        printf("SIGNALING curCarID that it can start; %d\n", curCarID);
        pthread_cond_broadcast(&condCanArriveNextCar);

        pthread_mutex_unlock(&mutex);
    }

    doSthBeforeExitInCar(thread_no);
    return NULL;
}


void parse_input(int argc, char **argv) {
    if (argc != 5) die_errno("Pass: \n- passengerCount \n- carCount \n- carCapacity \n- leftRidesCount\n");

    RC.passengerCount = atoi(argv[1]);
    RC.carCount = atoi(argv[2]);
    RC.carCapacity = atoi(argv[3]);
    RC.leftRidesCount = atoi(argv[4]);

    printf("passengerCount: %d, ", RC.passengerCount);
    printf("carCount: %d, ", RC.carCount);
    printf("carCapacity: %d, ", RC.carCapacity);
    printf("leftRidesCount: %d\n", RC.leftRidesCount);

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
    isRideFinished = 0;
    // leftCarsCount = RC.carCount;
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
    pthread_cond_destroy(&condNewCarToGetOffArrived);
    pthread_cond_destroy(&condEmptyCar);
    pthread_cond_destroy(&condFullCar);
    pthread_cond_destroy(&condCanArriveNextCar);
    pthread_cond_destroy(&condPassengerEnterCar);
    pthread_cond_destroy(&condFinishedRide);

    return 0;
}