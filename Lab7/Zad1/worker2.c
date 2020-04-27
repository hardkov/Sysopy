#include "my_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>

bool WORK;

long long msec_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL);   
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

void sigint_handler(int signo){
    WORK = false;
}

// preparing packages
int main(int argc, char* argv[]){
    int order_size = -1;
    int sm_id, sem_id;
    key_t sm_key, sem_key;
    int* sm_addr;
    int* to_prepare;
    int* to_send;
    int* first_free_idx;
    int* first_to_prepare;
    int* first_to_send;
    struct sembuf* sb = (struct sembuf*) calloc(1, sizeof(struct sembuf));

    sem_key = ftok("./main.c", 2);
    sm_key = ftok("./main.c", 1);
    WORK = true;
    
    // opening semaphores and attatching shared mem
    sm_id = shmget(sm_key, 0, 0);
    if(sm_id == -1){
        perror("shmget");
    }

    sem_id = semget(sem_key, 0, 0);
    if(sem_id == -1){
        perror("semget");
    }


    sm_addr = (int*) shmat(sm_id, NULL, 0);
    if(sm_addr == (int*) -1){
        perror("shmat");        // addr sm_addr -> array size 200, sm_addr+SM_ARR_SIZE -> int to_prepare, sm_addr_SM_ARR_SIZE+1 -> int to_send
    }                           // sm_addr + SM_ARR_SIZE + 2 -> int first_free_idx int first_to_prepare, int first_to_send

    // real work
    while(WORK){
        sb[0].sem_flg = 0;
        sb[0].sem_num = 0;    // blocking access if sem was 1, or waiting for access if sem was 1
        sb[0].sem_op = -1;
        if(semop(sem_id, sb, 1) == -1){
            perror("semop start");
        }

        // accessing shared memory
        to_prepare = sm_addr + SM_ARR_SIZE;
        to_send = sm_addr + SM_ARR_SIZE + 1;
        first_free_idx = sm_addr + SM_ARR_SIZE + 2;
        first_to_prepare = sm_addr + SM_ARR_SIZE + 3;
        first_to_send = sm_addr + SM_ARR_SIZE + 4;

        if(*first_to_prepare != -1){   
            order_size = sm_addr[*first_to_prepare]; 
            order_size *= 2;

            if(*first_to_send == -1) *first_to_send = *first_to_prepare;  // there was nothing to be send, now there is
            *first_to_prepare += 1;
            if(*first_free_idx == *first_to_prepare) *first_to_prepare = -1; // there is nothing to be prepared now
            *to_prepare -= 1;
            *to_send += 1;
            
            printf("%d %lld Przygotowalem zamowienie o wielkosci: %d\nLiczba zamówień do przygotowania: %d\nLiczba zamówień do wysłania: %d\n\n", 
                    getpid(), msec_timestamp(), order_size, *to_prepare, *to_send);
        }
        
        sb[0].sem_flg = 0;
        sb[0].sem_num = 0;   
        sb[0].sem_op = 1; // unblocking
        if(semop(sem_id, sb, 1) == -1){
            perror("semop start");
        }

        sleep(1);
    }

    shmdt(sm_addr);

    return 0;
}