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
#include <unistd.h>


bool WORK;

void sigint_handler(int signo){
    WORK = false;
}

int main(int argc, char* argv[]){
    int sm_fd, pack, prep, send;
    int* sm_addr;
    sem_t* sem_addr;
    int* to_prepare;
    int* to_send;
    int* first_free_idx;
    int* first_to_prepare;
    int* first_to_send;

    if(argc != 4){
        printf("Usage: %s prep_workers pack_workers send_workers\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    prep = atoi(argv[1]);
    pack = atoi(argv[2]);
    send = atoi(argv[3]);

    WORK = true;
    signal(SIGINT, sigint_handler);

    sem_addr = sem_open("Sems", O_CREAT, 00700, 1);

    sm_fd = shm_open("Orders", O_CREAT | O_RDWR, 00700);
    if(sm_fd == -1) perror("shm_open");
    
    if(ftruncate(sm_fd, SM_SIZE) == -1) perror("truncate");

    sm_addr = (int*) mmap(NULL, SM_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, sm_fd, 0);
    if(sm_addr == (int*) -1) perror("mmap");

    to_prepare = sm_addr + SM_ARR_SIZE;
    to_send = sm_addr + SM_ARR_SIZE + 1;
    first_free_idx = sm_addr + SM_ARR_SIZE + 2;
    first_to_prepare = sm_addr + SM_ARR_SIZE + 3;
    first_to_send = sm_addr + SM_ARR_SIZE + 4;

    *to_prepare = 0;
    *to_send = 0;
    *first_free_idx = 0;
    *first_to_prepare = -1;
    *first_to_send = -1;

    while(WORK){
        if(prep-- > 0 && fork() == 0){
            execl("./worker1", "worker1", NULL);
        }
        else if(pack-- > 0 && fork() == 0){
            execl("./worker2", "worker2", NULL);
        }
        else if(send-- > 0 && fork() == 0){
            execl("./worker3", "worker3", NULL);
        }
        sleep(1);
    }

    if(munmap((void*) sm_addr, SM_SIZE) == -1) perror("munmap");
    if(close(sm_fd) == -1) perror("close");
    if(shm_unlink("Orders") == -1) perror("shm_unlink");
    if(sem_close(sem_addr) == -1) perror("sem_close");
    if(sem_unlink("Sems") == -1) perror("sem_unlink");
    return 0;
}  