#include "common.h"

#define MAX_CYCLE_AND_WEIGHT_DIGITS 4

Belt* belt;
int shm_fd;
int workersCount;
char **cycles;
char **workers_pckg_weight;

void parse_input(int argc, char **argv){
    if(argc != 2) die_errno("Give me the number of workers\n");
    workersCount = (int) strtol(argv[1], NULL, 10);
    cycles = malloc(workersCount * sizeof(char *));
    workers_pckg_weight = malloc(workersCount * sizeof(char *));
    for(int i = 0; i < workersCount; i++){
        cycles[i] = malloc(MAX_CYCLE_AND_WEIGHT_DIGITS * sizeof(char));
        workers_pckg_weight[i] = malloc(MAX_CYCLE_AND_WEIGHT_DIGITS * sizeof(char));
    }
    for(int i = 0; i < workersCount; i++){
        scanf("%s", workers_pckg_weight[i]);
        scanf("%s", cycles[i]);
    }
    for(int i = 0; i < workersCount; i++)
        printf("weight: %s, cycles: %s\n", workers_pckg_weight[i], cycles[i]);
    // printf("max pckgs count: %s\n", max_pckgsCount_on_the_belt);
}

void free_memory(){
    for(int i = 0; i < workersCount; i++){
        free(cycles[i]);
        free(workers_pckg_weight[i]);
    }
    free(cycles);
    free(workers_pckg_weight);
}


void create_and_init_shm(){
    printf("creating shm\n");
    if((shm_fd = shm_open("/shared_memory", O_RDWR, 0)) < 0) die_errno("ERROR WITH OPENING MEMORY");
    if((belt = mmap(NULL, sizeof(Belt), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *) -1) die_errno("ERROR WITH GETTING BELT");
}

void create_and_init_semaphores(){
    semaphores[END_LINE_SEMAPHORE] = sem_open("/END_LINE_SEMAPHORE", O_RDWR , 0666);
    semaphores[TRUCK_SEMAPHORE] = sem_open("/TRUCK_SEMAPHORE", O_RDWR , 0666);
    semaphores[START_LINE_SEMAPHORE] = sem_open("/START_LINE_SEMAPHORE", O_RDWR , 0666);
}

void die_errno(char *message);

int main(int argc, char **argv) {
    parse_input(argc, argv);
    create_and_init_shm();
    create_and_init_semaphores();

    for(int i = 0; i < workersCount; i++){
        if(fork() == 0){
            package p;
            p.workerID = getpid();
            p.weight = (int) strtol(workers_pckg_weight[i], NULL, 10);
            int cyclesNo = (int) strtol(cycles[i], NULL, 10);
            if (cyclesNo != -1) {
                for (int j = 0; j < cyclesNo; j++)
                    putBox(belt, p);
                exit(0);
            } else {
                while(1) putBox(belt, p);
            }
        }
    }
    for (int i = 0; i < workersCount; i++) wait(NULL);
    free_memory();
}