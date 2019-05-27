#include "utils.h"

RollerCoaster RC;
pthread_t *psgTIDs, *carTIDs;
int carTIDsOffset; // car TIDs start from this offset
int *consequentIDs; // storing there IDs (at first - passenger IDs, then - car IDs)
int consequentIDsSize;

// current data
int curCarInd;
// int curPassengerCount;
int canEnterCar; // for passenger
int canPressStart;
int startIndex; // index in curCarPassengers telling which passenger will press start  
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
    free(consequentIDs);
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
        // printf("Waiting!\n");
        pthread_cond_signal(&condCanPressStart);

        pthread_cond_wait(&condPressedStart, &mutex);
    }
}

void ride(int thread_no) {    
    printStartedRide(thread_no);
    
    // let next car arrive
    // curCarInd = (curCarInd + 1 - carTIDsOffset) % RC.carCount + carTIDsOffset;
    curCarInd = (curCarInd + 1) % RC.carCount;
    
    pthread_cond_broadcast(&condCanArriveNextCar);

    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = 1e6 * (rand() % 10);
    printf("RIDING for %ld nsec.\n", (long) spec.tv_nsec);
    if(nanosleep(&spec, NULL) < 0) die_errno("ride: nanosleep");

    printFinishedRide(thread_no);

    if(thread_no == RC.carCount + carTIDsOffset - 1){
        // printf("decrementing\n");
        RC.leftRidesCount--;
        // printf("left rides %d\n", RC.leftRidesCount);
        printf("\n");
    }

    // for(int i = 0; i < RC.carCapacity; i++)
        // pthread_cond_signal(&condFinishedRide);
    pthread_cond_broadcast(&condFinishedRide);
}

// passenger functions
int addPassengerToCar(int thread_no) {
    // printf("TID %d: Left cars count: %d\n", thread_no, leftCarsCount);
    if(RC.cars[curCarInd].passengersCount == RC.carCapacity || RC.leftRidesCount == 0 ||
         leftCarsCount == 0) return -1; // die_errno("Too many passengers");
    
    int passengersCount = RC.cars[curCarInd].passengersCount;
    RC.cars[curCarInd].passengersIDs[passengersCount] = thread_no;
    RC.cars[curCarInd].passengersCount++;
    // curPassengerCount++;
    if(RC.cars[curCarInd].passengersCount == RC.carCapacity) {
        printf("Car %d is full!\n", RC.cars[curCarInd].ID);
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

void doSthBeforeExitInPassenger(int thread_no) {
    printFinishedThread(thread_no, "PASSENGER");
    pthread_mutex_unlock(&mutex);
}

void tryToPressStart(int thread_no){
    // printf("TID: %d: Start index before: %d\n", thread_no, startIndex);
    if(startIndex == -1) pthread_cond_wait(&condKnownNewStartIndex, &mutex);
    // printf("TID: %d: Passed\n", thread_no);
    if(startIndex == -1) {
        printf("TID %d: DYING.....\n", thread_no);
        die_errno("Start index = -1 :(");
    }
    // printf("Incrementing!\n");
    passengersKnowingNewStartIndex++;
    // printf("passengersKnowingNewStartIndex: %d\n", passengersKnowingNewStartIndex);
    if(passengersKnowingNewStartIndex == RC.carCapacity){
        printf("Signaling\n");
        pthread_cond_signal(&condAllReceivedNewStartIndex);
    }
    if(RC.cars[curCarInd].passengersIDs[startIndex] == thread_no) {
        printf("Waiting until can press start\n");
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
    int thread_no = *(int *) thread_num;
    int myCurrentCar = -1;

    pthread_cond_broadcast(&condEmptyCar);

    while(1){

        pthread_mutex_lock(&mutex);

        // wait until passenger can enter the car
        while(!canEnterCar){
            pthread_cond_wait(&condPassengerEnterCar, &mutex);

            // printf("Left cars count: %d\n", leftCarsCount);
            if(leftCarsCount == 0 || RC.leftRidesCount == 0) break;

            if(RC.cars[curCarInd].passengersCount >= RC.carCapacity) continue;
        }
        // printf("V2: Left cars count: %d\n", leftCarsCount);
        if(leftCarsCount /* RC.leftRidesCount */ == 0) break;

        if((myCurrentCar = addPassengerToCar(thread_no)) < 0) {
            // printf("Adding a passenger to the car failed. Try again!\n");
            pthread_mutex_unlock(&mutex);
            continue;
        }

        tryToPressStart(thread_no);

        // printf("How many times would you try to do it???\n");
        // wait for people to get off the car        
        if(RC.cars[curCarInd].passengersCount > 0) {
            pthread_cond_wait(&condOpenDoor, &mutex);
            if(myCurrentCar == -1) die_errno("myCurrentCar = -1");
            while(curCarInd != myCurrentCar) {
                // printf("CUR CAR IND: %d, thread_no %d\n", curCarInd, thread_no);
                // printf("What's the problem, Houston? %d\n", RC.cars[curCarInd].ID);
                pthread_cond_wait(&condOpenDoor, &mutex);
            }
            printf("TID %d: Leaving car!\n", thread_no);
            myCurrentCar = rmvPassengerFromCar(thread_no);
            if(RC.cars[curCarInd].passengersCount == 0) {
                clearPassengersIDsFromCar();

                printCurrentCarsState(RC);
                
                // signaling empty car
                pthread_cond_signal(&condEmptyCar);
            }
        }
// printf("Here\n");
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
    canEnterCar = 1;

    pthread_cond_broadcast(&condPassengerEnterCar);

    printFinishedThread(thread_no, "CAR");

    pthread_mutex_unlock(&mutex);
}

void waitUntilCarIsEmpty(int thread_no) {
    do{
        pthread_cond_wait(&condEmptyCar, &mutex);
    } while(RC.cars[curCarInd].ID != thread_no);
}

void* car(void *thread_num) {
    int thread_no = *(int *) thread_num;
    
    pthread_cond_broadcast(&condCanArriveNextCar);

    while(1) {

        pthread_mutex_lock(&mutex);

        // wait for your turn

        while(thread_no != RC.cars[curCarInd].ID) {
            // printf("cur Car Ind : %d\n", curCarInd);

            pthread_cond_wait(&condCanArriveNextCar, &mutex);
            
            if(RC.leftRidesCount == 0) break;
        }

        printOpeningDoor(thread_no);

        pthread_cond_broadcast(&condOpenDoor);

        printf("Waiting for passnegers to leave\n");

        if(RC.leftRidesCount == 0) {
            if(thread_no == RC.cars[curCarInd].ID && RC.cars[curCarInd].passengersCount > 0)
                waitUntilCarIsEmpty(thread_no);
            break;
        }

        if(RC.cars[thread_no - carTIDsOffset].passengersCount > 0) {
            waitUntilCarIsEmpty(thread_no);
        }


        printf("OK. Left\n");

        // let people enter the car
        canEnterCar = 1;
        for(int i = 0; i < RC.carCapacity; i++){
            pthread_cond_signal(&condPassengerEnterCar);
        }

        pthread_cond_wait(&condFullCar, &mutex);
        canEnterCar = 0;
        
        // car is full, so now start can be pressed

        waitForPressingStart(thread_no);
        startIndex = -1;

        printf("No nie doczekam sie ;-;\n");
        
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
    consequentIDs = malloc(consequentIDsSize * sizeof(int));
    for (int i = 0; i < consequentIDsSize; i++)
        consequentIDs[i] = i;
    
    carTIDsOffset = RC.totalPassengerCount + 10200;

    psgTIDs = malloc(RC.totalPassengerCount * sizeof(pthread_t));

    curCarInd = 0;
    canEnterCar = 0;
    canPressStart = 0;
    startIndex = -1;
    passengersKnowingNewStartIndex = 0;
    leftCarsCount = RC.carCount;
}

void createPassengerThreads() {
    for (int i = 0; i < RC.totalPassengerCount; i++)
        if(pthread_create(&psgTIDs[i], NULL, &passenger, &consequentIDs[i]) < 0) 
            die_errno("creating passenger threads");
}

void createCarThreads() {
    // increment my threads IDs 
    for (int i = 0; i < RC.carCount; i++) {
        consequentIDs[i] += carTIDsOffset;
        RC.cars[i].ID = consequentIDs[i];
    }

    for (int i = 0; i < RC.carCount; i++)
        if(pthread_create(&carTIDs[i], NULL, &car, &consequentIDs[i]) < 0) 
            die_errno("creating car threads");
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