#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


int recieved;
int number_of_signals;

void sig1_handler_kill(int sig, siginfo_t* info, void* ucontext_t){
    recieved++;
}
void sig1_handler_sigqueue(int sig, siginfo_t* info, void* ucontext_t){
    recieved++;
    printf("Wartość przesłana z sygnałem: %d\n", info -> si_value.sival_int);
}

void sig2_handler(int sig, siginfo_t* info, void* ucontext_t){
    printf("Otrzymalem %d sygnalow a powinienem otrzymac %d\n", recieved, number_of_signals);
    exit(0);
}

void kill_version(int pid, int number_of_signals){
    while(number_of_signals > 0){
        kill(pid, SIGUSR1);
        number_of_signals--;
    }

    kill(pid, SIGUSR2);
}

void sigqueue_version(int pid, int number_of_signals){
    union sigval value;
    value.sival_int = 0; 

    while(number_of_signals > 0){
        sigqueue(pid, SIGUSR1, value);
        number_of_signals--;
    }

    sigqueue(pid, SIGUSR2, value);
}

void sigrt_version(int pid, int number_of_signals){
    while(number_of_signals > 0){
        kill(pid, SIGRTMIN);
        number_of_signals--;
    }

    kill(pid, SIGRTMAX);
}

int main(int argc, char* argv[]){
    if(argc != 4){
        printf("usage: ./sender pid number_of_signals mode\n");
        exit(0);
    }

    int pid, sig1, sig2;
    char* mode = (char*) calloc(strlen(argv[3]), sizeof(char));

    printf("%d\n", getpid());

    recieved = 0;
    pid = atoi(argv[1]);
    number_of_signals = atoi(argv[2]);
    strcpy(mode, argv[3]);

    if(strcmp(mode, "sigrt") == 0){
        sig1 = SIGRTMIN;
        sig2 = SIGRTMAX;
    }
    else{
        sig1 = SIGUSR1;
        sig2 = SIGUSR2;
    }

    struct sigaction act;
    sigset_t mask;

    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);

    act.sa_sigaction = sig2_handler;
    sigaction(sig2, &act, NULL);
    
    sigfillset(&mask);
    sigdelset(&mask, sig1);
    sigdelset(&mask, sig2);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    
    if(strcmp(mode, "kill") == 0){
        act.sa_sigaction = sig1_handler_kill;
        sigaction(sig1, &act, NULL);

        kill_version(pid, number_of_signals);
    }
    else if(strcmp(mode, "sigqueue") == 0){
        act.sa_sigaction = sig1_handler_sigqueue;
        sigaction(sig1, &act, NULL);

        sigqueue_version(pid, number_of_signals);
    }
    else if(strcmp(mode, "sigrt") == 0){
        act.sa_sigaction = sig1_handler_kill;
        sigaction(sig1, &act, NULL);

        sigrt_version(pid, number_of_signals);
    }
    else printf("wrong arg");

    while(1){
        sleep(1);
    }
    
    free(mode);
    return 0;
}