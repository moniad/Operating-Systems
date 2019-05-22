#include "common.h"

Belt *belt;
Truck truck;
int max_pckgsCount_on_the_belt;
int capacity;
int max_pckgsWeight_on_the_belt;
int shm_fd;

void die_errno(char *message);

void SIGINThandler(int signum);

void clearEverything();

void run();

void parse_input(int argc, char **argv){
    if(argc != 4) die_errno("Give me:\n 1) X - the capacity of the truck\n 2) K - max no of packages on the belt\n 3) M - max total sum of packages' weight\n");
    capacity = (int) strtol(argv[1], NULL, 10);
    max_pckgsCount_on_the_belt = (int) strtol(argv[2], NULL, 10);
    max_pckgsWeight_on_the_belt = (int) strtol(argv[3], NULL, 10);
    // printf("%d, %d, %d\n", capacity, max_pckgsCount_on_the_belt, max_pckgsWeight_on_the_belt);

    if (capacity <= 0 || max_pckgsCount_on_the_belt <= 0 || max_pckgsWeight_on_the_belt <= 0)
        die_errno("Wrong arguments");
    if (max_pckgsCount_on_the_belt > MAX_PCKGS_NO)
        die_errno("Too many packages");
}

void create_and_init_shm(){
    printf("creating shm\n");
    if((shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR , 0666)) < 0) die_errno("shm_fd");
    if (ftruncate(shm_fd, sizeof(Belt)) < 0) die_errno("ftruncate");

    if((belt = mmap(NULL, sizeof(Belt), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *) -1)
        die_errno("mmap");
}

void create_and_init_semaphores(){
    semaphores[END_LINE_SEMAPHORE] = sem_open("/END_LINE_SEMAPHORE", O_RDWR | O_CREAT, 0666, 0);
    semaphores[TRUCK_SEMAPHORE] = sem_open("/TRUCK_SEMAPHORE", O_RDWR | O_CREAT, 0666, truck.maxBoxCount);
    semaphores[START_LINE_SEMAPHORE] = sem_open("/START_LINE_SEMAPHORE", O_RDWR | O_CREAT, 0666, 1);
}


int main(int argc, char **argv) {
    parse_input(argc, argv);
    create_and_init_shm();

    belt->maxWeight = max_pckgsWeight_on_the_belt;
    belt->maxBoxes = max_pckgsCount_on_the_belt;
    belt->currentBoxes = 0;
    belt->currentWeight = 0;
    belt->currentBoxInLine = 0;
    truck.maxBoxCount = capacity;

    create_and_init_semaphores();
    
    atexit(clearEverything);

    signal(SIGINT, SIGINThandler);

    printf("Przyjechała pusta ciężarówka\n");

    run();
}

void clearEverything() {
    sem_unlink("/END_LINE_SEMAPHORE");
    sem_unlink("/TRUCK_SEMAPHORE");
    sem_unlink("/START_LINE_SEMAPHORE");
    shm_unlink("/shared_memory");
}

void run() {
    int currentTaken = 0;
    int i = 0;
    while (1) {
        printf("Czekam na załadowanie paczki\n");
        if (belt->truckEnded == 1) {
            printf("Last Truck %d => Weight:%d | Boxes:%d\n", i, truck.currentWeight, truck.currentBoxNumber);
            exit(0);
        }
        takeSemaphore(END_LINE_SEMAPHORE);
        if (belt->truckEnded == 1) {
            printf("Last Truck %d => Weight:%d | Boxes:%d\n", i, truck.currentWeight, truck.currentBoxNumber);
            exit(0);
        }
        takeSemaphore(START_LINE_SEMAPHORE);
        package p = belt->line[currentTaken];
        truck.currentWeight += p.weight;
        truck.currentBoxNumber++;
        belt->currentBoxes--;
        belt->currentWeight -= p.weight;
        struct timeval end;
        gettimeofday(&end, NULL);
        printf("Proces: PID:%d załadował paczkę: WAGA: %d | TIME_STAMP: %s | TIMEDIFF: %ld | MASA CIĘŻARÓWKI: %d | WOLNE MIEJSCA: %d\n",
               p.workerID, p.weight, get_date_time(), end.tv_usec - p.loadTime.tv_usec, truck.currentWeight,
               truck.maxBoxCount - truck.currentBoxNumber);
        currentTaken++;
        currentTaken %= MAX_PCKGS_NO;

        if (truck.currentBoxNumber == truck.maxBoxCount) {
            printf("Brak miejsca w ciężarówce. Odjazd!\n");
            printf("CIĘŻARÓWKA NR %d: Waga: %d | Załadowane jednostki: %d\n", i, truck.currentWeight, truck.currentBoxNumber);
            truck.currentWeight = 0;
            truck.currentBoxNumber = 0;
            printf("Przyjechała pusta ciężarówka\n");
            i++;
            for(int j = 0; j < truck.maxBoxCount; j++)
                releaseSemaphore(TRUCK_SEMAPHORE);
        }
        releaseSemaphore(START_LINE_SEMAPHORE);
    }
}

void SIGINThandler(int signum) {
    belt->truckEnded = 1;
    printf("Last => Weight:%d | Boxes:%d\n", truck.currentWeight, truck.currentBoxNumber);
    exit(0);
}