#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int PROGRAM_STATE; // -1 paused 1 unpaused

void int_handler(int signum){
    printf("\nOdebrano sygnał SIGINT\n");
    exit(0);
}

void stp_handler(int sig_no){
    printf("\nOczekuję na CTRL+Z - kontynuacja albo CTR+C - zakończenie programu\n");
    if(PROGRAM_STATE == -1) PROGRAM_STATE = 1;
    else PROGRAM_STATE = -1;
}

int main(int argc, char* argv[]){
    PROGRAM_STATE = 1;
    printf("%d\n", getpid());
    
    struct sigaction act;
    act.sa_handler = stp_handler;
    sigemptyset(&act.sa_mask);  
    act.sa_flags = 0;
    
    sigaction(SIGTSTP, &act, NULL);
    signal(SIGINT, int_handler);

    while(1){
        system("ls");
        sleep(1);
        if(PROGRAM_STATE == -1) pause();
    };

    return 0;
}
