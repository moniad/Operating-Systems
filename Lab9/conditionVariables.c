#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_t tid[2];
int counter;
int x = 0, y = 6;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condXGreaterThanY = PTHREAD_COND_INITIALIZER;
int stopChangingX = 0;
pthread_cond_t condXChange = PTHREAD_COND_INITIALIZER;

void die_errno(char *message) {
    printf("ERROR: %s\n", message);
    exit(1);
}

void* changeX(void *thread_num)
{
    pthread_mutex_lock(&mutex);
    while(!stopChangingX){
        int thread_no = *(int *) thread_num;

        printf("\n Incrementation by %d started\n", thread_no);
        
        x++;
        printf("NOW: x = %d, y = %d\n", x, y);
        if(x > y){
            pthread_cond_broadcast(&condXGreaterThanY);
            stopChangingX = 1;
        }

        printf("\n Incrementation by %d finished\n", thread_no);
        sleep(2);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* waitUntilXGreaterThanY(void *thread_num)
{
    int thread_no = *(int *) thread_num;
    pthread_mutex_lock(&mutex);

    printf("\n Waiting: %d \n", thread_no);
    
    printf("INITIAL STATE: x = %d, y = %d\n", x, y);
    
    while (x <= y){
        pthread_cond_wait(&condXGreaterThanY, &mutex);
        sleep(2);
    }

    printf("\n FINALLY x > y!!!! \n");

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(void)
{
    int id = 0;
    if(pthread_create(&(tid[0]), NULL, &waitUntilXGreaterThanY, (void *) &id)< 0) 
        die_errno("creating first thread");
    id++;
    if(pthread_create(&(tid[1]), NULL, &changeX, (void *) &id) < 0) 
        die_errno("creating second thread");

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_mutex_destroy(&mutex);

    return 0;
}