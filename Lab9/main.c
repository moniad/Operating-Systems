#include "utils.h"

RollerCoaster RC;

void pressStart(int thread_no){ // the last person who enters the lift presses the button
    printPressedStart(thread_no);
    pthread_cond_broadcast(&condPressedStart);
}

int getStartIndex(){
    int a = rand() % RC.carCapacity;
    // printf("Returning such random index: %d\n", a);
    return a;
}

void waitForPressingStart(int thread_no){
    if(RC.cars[curCarInd].ID == thread_no){
        passengersKnowingNewStartIndex = 0;
        startIndex = getStartIndex();
        pthread_cond_broadcast(&condKnownNewStartIndex);

        if(passengersKnowingNewStartIndex != RC.carCapacity){
            pthread_cond_wait(&condAllReceivedNewStartIndex, &mutex);
        }

        pthread_cond_signal(&condCanPressStart);

        pthread_cond_wait(&condPressedStart, &mutex);
    }
}

void ride(int thread_no) {    
    printStartedRide(thread_no);
    
    // let next car arrive
    // curCarID = (curCarID + 1 - carTIDsOffset) % RC.carCount + carTIDsOffset;
    curCarInd = (curCarInd + 1) % RC.carCount;
    
    pthread_cond_broadcast(&condCanArriveNextCar);

    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = 1e6 * (rand() % 10);
    printf("RIDING for %ld nsec.\n", (long) spec.tv_nsec);
    if(nanosleep(&spec, NULL) < 0) die_errno("ride: nanosleep");

    printFinishedRide(thread_no);

    if(thread_no == RC.carCount + carTIDsOffset - 1){
        RC.leftRidesCount--;
        printf("\n");
    }

    pthread_cond_broadcast(&condFinishedRide);
}

// passenger functions
int addPassengerToCar(int thread_no) {
    if(RC.cars[curCarInd].passengersCount == RC.carCapacity || RC.leftRidesCount == 0 ||
         leftCarsCount == 0) return -1;
    
    int passengersCount = RC.cars[curCarInd].passengersCount;
    RC.cars[curCarInd].passengersIDs[passengersCount] = thread_no;
    RC.cars[curCarInd].passengersCount++;

    if(RC.cars[curCarInd].passengersCount == RC.carCapacity) {
        printCurrentCarsState(RC);
        pthread_cond_broadcast(&condFullCar);
    }
    printEnteringCar(thread_no, RC.cars[curCarInd].passengersCount);

    return curCarInd;
}

int rmvPassengerFromCar(int thread_no) {
    RC.cars[curCarInd].passengersCount--;
    printLeavingCar(thread_no, RC.cars[curCarInd].passengersCount);
    return -1;
}

void doSthBeforeExitInPassenger(/*void *arg*/int thread_no) {
    printFinishedThread(/**(int*)(arg)*/ thread_no, "PASSENGER");
    pthread_mutex_unlock(&mutex);
}

void tryToPressStart(int thread_no){
    if(startIndex == -1)
        pthread_cond_wait(&condKnownNewStartIndex, &mutex);
    if(startIndex == -1) {
        printf("TID %d: DYING.....\n", thread_no);
        die_errno("Start index = -1 :(");
    }
    passengersKnowingNewStartIndex++;
    
    if(passengersKnowingNewStartIndex == RC.carCapacity){
        pthread_cond_signal(&condAllReceivedNewStartIndex);
    }
    if(RC.cars[curCarInd].passengersIDs[startIndex] == thread_no) {
        pthread_cond_wait(&condCanPressStart, &mutex);
        pressStart(thread_no);
    }
}

void clearPassengersIDsFromCar() {
    for(int i = 0; i < RC.cars[i].passengersCount; i++) {
        RC.cars[curCarInd].passengersIDs[i] = -1;
    }
}

void* passenger(void *thread_num) {
    // pthread_cleanup_push(doSthBeforeExitInPassenger, thread_num);
    // if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) < 0)
    //     die_errno("setting cancel state");
    // if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) < 0)
    //     die_errno("setting cancel type");

    int thread_no = *(int *) thread_num;
    printf("thread_no %d\n", thread_no);

    int myCurrentCar = -1;

    pthread_cond_broadcast(&condEmptyCar);

    while(1){

        pthread_mutex_lock(&mutex);

        // wait until passenger can enter the car
        while(!canEnterCar){
            printf("waiting!\n");
            pthread_cond_wait(&condPassengerEnterCar, &mutex);

            if(leftCarsCount == 0 || RC.leftRidesCount == 0) break;

            if(RC.cars[curCarInd].passengersCount >= RC.carCapacity) continue;
        }

        if(leftCarsCount == 0) {
            break;
        }

        if((myCurrentCar = addPassengerToCar(thread_no)) < 0) {
            pthread_mutex_unlock(&mutex);
            continue;
        }

        tryToPressStart(thread_no);

        // wait for people to get off the car        
        if(RC.cars[curCarInd].passengersCount > 0) {

            if(myCurrentCar == -1) die_errno("myCurrentCar = -1");

            do {  
                pthread_cond_wait(&condOpenDoor, &mutex);
                if(RC.leftRidesCount == 0) {
                    break;
                }
            } while(curCarInd != myCurrentCar);

            if(RC.leftRidesCount == 0) {
                pthread_cond_signal(&condEmptyCar);
                break;
            }

            myCurrentCar = rmvPassengerFromCar(thread_no);
            if(RC.cars[curCarInd].passengersCount == 0) {
                clearPassengersIDsFromCar();

                printCurrentCarsState(RC);
                
                // signaling empty car
                pthread_cond_signal(&condEmptyCar);
            }
        }
        
        pthread_mutex_unlock(&mutex);
    }

    doSthBeforeExitInPassenger(thread_no);
    return NULL;
}

// car functions

void doSthBeforeExitInCar(int thread_no) {
    leftCarsCount--;

    // tell every car that it can arrive so that all car threads can finish
    pthread_cond_broadcast(&condCanArriveNextCar);

    printFinishedThread(thread_no, "CAR");

    if(leftCarsCount == 0){
        // to finish all passengers waiting to enter the car
        canEnterCar = 1; 
        pthread_cond_broadcast(&condPassengerEnterCar); 
        // to finish all passengers inside cars
        pthread_cond_broadcast(&condOpenDoor);
    }
    //     for (int i = 0; i < RC.totalPassengerCount; i++){
    //         if(pthread_cancel(psgTIDs[i]) < 0) die_errno("Canceling passenger threads");
    //     }
    // }
    pthread_mutex_unlock(&mutex);
}

void waitUntilCarIsEmpty(int thread_no) {
    printf("functioN!\n");
    do{
        pthread_cond_wait(&condEmptyCar, &mutex);
    } while(RC.cars[curCarInd].ID != thread_no);
}

void* car(void *thread_num) {
    int thread_no = *(int *) thread_num;

    printf("thread_no %d\n", thread_no);
    
    pthread_cond_broadcast(&condCanArriveNextCar);

    while(1) {

        pthread_mutex_lock(&mutex);

        // wait for your turn
        while(thread_no != RC.cars[curCarInd].ID) {
            pthread_cond_wait(&condCanArriveNextCar, &mutex);
            if(RC.leftRidesCount == 0) break;
        }

        if(RC.leftRidesCount == 0) {
            break;
        }
        
        printOpeningDoor(thread_no);
        pthread_cond_broadcast(&condOpenDoor);

        if(RC.cars[thread_no - carTIDsOffset].passengersCount > 0) {
            waitUntilCarIsEmpty(thread_no);
        }

        // let people enter the car
        canEnterCar = 1;
        printf("TID: %d: Okay, you can now enter the car.\n", thread_no);
        for(int i = 0; i < RC.carCapacity; i++){
            pthread_cond_signal(&condPassengerEnterCar);
        }

        pthread_cond_wait(&condFullCar, &mutex);
        canEnterCar = 0;
        
        // car is full, so now start can be pressed
        waitForPressingStart(thread_no);
        startIndex = -1;
        
        printClosingDoor(thread_no);

        ride(thread_no);

        pthread_mutex_unlock(&mutex);
    }

    doSthBeforeExitInCar(thread_no);
    return NULL;
}

void parse_input(int argc, char **argv) {
    if (argc != 5) die_errno("Pass: \n- passengerCount \n- carCount \n- carCapacity \n- leftRidesCount\n");

    RC.totalPassengerCount = atoi(argv[1]);
    RC.carCount = atoi(argv[2]);
    RC.carCapacity = atoi(argv[3]);
    RC.leftRidesCount = atoi(argv[4]);
    RC.cars = malloc(RC.carCount * sizeof(Car));

    for(int i = 0; i < RC.carCount; i++){
        RC.cars[i].ID = -1;
        RC.cars[i].passengersCount = 0;
        RC.cars[i].passengersIDs = malloc(RC.totalPassengerCount * sizeof(int));
        
        for(int j = 0; j < RC.cars[i].passengersCount; i++){
            RC.cars[i].passengersIDs[j] = 0;
        }
    }

    printf("totalPassengerCount: %d, ", RC.totalPassengerCount);
    printf("carCount: %d, ", RC.carCount);
    printf("carCapacity: %d, ", RC.carCapacity);
    printf("leftRidesCount: %d\n", RC.leftRidesCount);

    carTIDs = malloc(RC.carCount * sizeof(pthread_t));

    consequentIDsSize = RC.totalPassengerCount > RC.carCount ? RC.totalPassengerCount : RC.carCount;
    passengerConsequentIDs = malloc(consequentIDsSize * sizeof(int));
    carConsequentIDs = malloc(consequentIDsSize * sizeof(int));    

    carTIDsOffset = RC.totalPassengerCount + 10200;

    for (int i = 0; i < consequentIDsSize; i++) {
        passengerConsequentIDs[i] = i;
        carConsequentIDs[i] = i + carTIDsOffset;
    }

    psgTIDs = malloc(RC.totalPassengerCount * sizeof(pthread_t));

    curCarInd = 0;
    canEnterCar = 0;
    canPressStart = 0;
    startIndex = -1;
    passengersKnowingNewStartIndex = 0;
    leftCarsCount = RC.carCount;
}

void createPassengerThreads() {
    for (int i = 0; i < RC.totalPassengerCount; i++) {
        if(pthread_create(&psgTIDs[i], NULL, &passenger, &passengerConsequentIDs[i]) < 0) 
            die_errno("creating passenger threads");
        printf("Created a passenger!\n");
    }
}

void createCarThreads() {
    for (int i = 0; i < RC.carCount; i++) {
        RC.cars[i].ID = carConsequentIDs[i];
        if(pthread_create(&carTIDs[i], NULL, &car, &carConsequentIDs[i]) < 0) 
            die_errno("creating car threads");
        printf("Created a car!\n");
    }
}

void waitForPassengers(){
    for (int i = 0; i < RC.totalPassengerCount; i++)
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

    doTheCleanUp();

    return 0;
}