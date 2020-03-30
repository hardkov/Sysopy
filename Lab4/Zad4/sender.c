#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int usr1_recieved;
int number_of_signals;

void usr1_handler(int sig, siginfo_t* info, void* ucontext_t){
    sender_pid = info -> si_pid;
    usr1_recieved++;
}

void usr2_handler(){
    printf("Otrzymalem %d sygnalow SIGUSR1 a powinienem otrzymac %d\n", usr1_recieved, number_of_signals);
    exit(0);
}

int main(int argc, char* argv[]){
    int pid, i;
    sigset_t mask;
    struct sigaction act;

    usr1_recieved = 0;
    pid = atoi(argv[1]);
    number_of_signals = atoi(argv[2]);

    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = usr1_handler;
    sigaction(SIGUSR1, &act, NULL);

    act.sa_sigaction = usr2_handler;
    sigaction(SIGUSR2, &act, NULL);

    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGUSR2);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    printf("%d\n", getpid());

    i = number_of_signals;
    while(i > 0){
        kill(pid, SIGUSR1);
        i--;
    }
    kill(pid, SIGUSR2);

    while(1){
        sleep(1);
    }
    
    return 0;
}