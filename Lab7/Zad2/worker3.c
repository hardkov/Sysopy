#define _POSIX_C_SOURCE 200809L
#include "my_conf.h"
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

bool WORK;

void sigint_handler(int signo){
    WORK = false;
}

long long msec_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL);   
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

int main(int argc, char* argv[]){
    int sm_fd, seed, order_size;
    time_t tt;
    sem_t* sem_addr;
    int* sm_addr;
    int* to_prepare;
    int* to_send;
    int* first_free_idx;
    int* first_to_prepare;
    int* first_to_send;


    WORK = true;
    signal(SIGINT, sigint_handler);
    seed = time(&tt);
    srand(seed);

    sem_addr = sem_open("Sems", O_RDWR);
    if(sem_addr == SEM_FAILED) perror("sem_open");

    sm_fd = shm_open("Orders", O_RDWR, 0);
    if(sm_fd == -1) perror("shm_open");

    sm_addr = (int*) mmap(NULL, SM_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, sm_fd, 0);

    while(WORK){
        sem_wait(sem_addr);

        to_prepare = sm_addr + SM_ARR_SIZE;
        to_send = sm_addr + SM_ARR_SIZE + 1;
        first_free_idx = sm_addr + SM_ARR_SIZE + 2;
        first_to_prepare = sm_addr + SM_ARR_SIZE + 3;
        first_to_send = sm_addr + SM_ARR_SIZE + 4;

        if(*first_to_send != -1){
            order_size = sm_addr[*first_to_send];
            order_size *= 3;

            *first_to_send += 1;
            if(*first_to_prepare == *first_to_send || 
                *first_free_idx == *first_to_send) *first_to_send = -1; // there is nothing to send now
            *to_send -= 1;    

            printf("%d %lld Wysłałem zamowienie o wielkosci : %d\nLiczba zamówień do przygotowania: %d\nLiczba zamówień do wysłania: %d\n\n", 
                    getpid(), msec_timestamp(), order_size, *to_prepare, *to_send);
        }

        sem_post(sem_addr);

        sleep(1);
    }

    if(munmap((void*) sm_addr, SM_SIZE) == -1) perror("munmap");
    if(close(sm_fd) == -1) perror("close");
    if(sem_close(sem_addr) == -1) perror("sem_close");

    return 0;
}  