#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


int recieved;
int sender_pid;

void sig1_handler(int sig, siginfo_t* info, void* ucontext_t){
    sender_pid = info -> si_pid;
    recieved++;
}

void sig2_handler_kill(int sig, siginfo_t* info, void* ucontext_t){
    int i, sig1, sig2;

    if(sig == SIGUSR2){
        sig1 = SIGUSR1;
        sig2 = SIGUSR2;
    } 
    else if(sig == SIGRTMAX){
        sig1 = SIGRTMIN; 
        sig2 = SIGRTMAX;
    }
    else printf("error");
    
    i = recieved;
    while(i > 0){
        kill(sender_pid, sig1);
        i--;
    }
    kill(sender_pid, sig2);
    printf("Otrzymalem %d sygnalow \n", recieved);
    exit(0);
}

void sig2_handler_sigqueue(int sig, siginfo_t* info, void* ucontext_t){
    int i;
    union sigval value;
    for(i = 0; i < recieved; i++){
        value.sival_int = i;
        sigqueue(sender_pid, SIGUSR1, value);
    }

    sigqueue(sender_pid, SIGUSR2, value);
    printf("Otrzymalem %d sygnalow \n", recieved);
    exit(0);
}

int main(int argc, char* argv[]){
    if(argc < 2){
        printf("usage: ./catcher mode\n");
        exit(0);
    }

    recieved = 0;
    struct sigaction act;
    sigset_t mask;
    char* mode = (char*) calloc(strlen(argv[1]), sizeof(char));
    int sig1, sig2;

    strcpy(mode, argv[1]);

    if(strcmp(mode, "sigrt") == 0){
       
        sig1 = SIGRTMIN;
        sig2 = SIGRTMAX;
    }
    else{
        
        sig1 = SIGUSR1;
        sig2 = SIGUSR2;
    }

    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = sig1_handler;
    sigaction(sig1, &act, NULL);

    sigfillset(&mask);
    sigdelset(&mask, sig1);
    sigdelset(&mask, sig2);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    if(strcmp(mode, "kill") == 0){
        
        act.sa_sigaction = sig2_handler_kill;
        sigaction(sig2, &act, NULL);
    }
    else if(strcmp(mode, "sigqueue") == 0){
       
        act.sa_sigaction = sig2_handler_sigqueue;
        sigaction(sig2, &act, NULL);
    }
    else if(strcmp(mode, "sigrt") == 0){
        
        act.sa_sigaction = sig2_handler_kill;
        sigaction(sig2, &act, NULL);
    }
    else printf("wrong arg");

    printf("%d\n", getpid());
    while(1){
        sleep(1);
    }

    return 0;
}