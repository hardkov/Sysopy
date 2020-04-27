#include "my_conf.h"
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


union semun {
            int val;
            struct semid_ds *buf;
            unsigned short  *array;
};

bool WORK;

void sigint_handler(int signo){
    WORK = false;
}

int main(int argc, char* argv[]){
    key_t sm_key, sem_key;
    int sm_id, sem_id;
    int prep, send, pack;
    int* sm_addr;
    int* first_to_prepare;
    int* first_to_send;
    union semun arg;

    if(argc != 4){
        printf("Usage: %s prep_workers pack_workers send_workers\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    prep = atoi(argv[1]);
    pack = atoi(argv[2]);
    send = atoi(argv[3]);

    WORK = true;
    signal(SIGINT, sigint_handler);

    sem_key = ftok("./main.c", 2);
    sm_key = ftok("./main.c", 1);

    sm_id = shmget(sm_key, SM_SIZE, IPC_CREAT | 00700);
    if(sm_id == -1) perror("sm_id");

    sem_id = semget(sem_key, 1, IPC_CREAT | 00700);
    if(sem_id == -1)perror("semget");
    arg.val = 1;
    semctl(sem_id, 0, SETVAL, arg);

    sm_addr = shmat(sm_id, NULL, 0);
    first_to_prepare = sm_addr + SM_ARR_SIZE + 3;
    first_to_send = sm_addr + SM_ARR_SIZE + 4;

    *first_to_prepare = -1;
    *first_to_send = -1;

    while(WORK){
        if(prep-- > 0 && fork() == 0) execl("./worker1", "worker1", NULL);
        else if(pack-- > 0 && fork() == 0) execl("./worker2", "worker2", NULL);
        else if(send-- > 0 && fork() == 0) execl("./worker3", "worker3", NULL);
        sleep(1);
    }

    shmdt(sm_addr);
    shmctl(sm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID, arg);

    return 0;
}   