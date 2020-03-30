#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int usr1_recieved;
int sender_pid;

void usr1_handler(int sig, siginfo_t* info, void* ucontext_t){
    sender_pid = info -> si_pid;
    usr1_recieved++;
}

void usr2_handler(){
    int i = usr1_recieved;

    while(i > 0){
        kill(sender_pid, SIGUSR1);
        i--;
    }
    kill(sender_pid, SIGUSR2);
    printf("Otrzymalem %d sygnalow SIGUSR1\n", usr1_recieved);
    exit(0);
}

int main(int argc, char* argv[]){
    usr1_recieved = 0;
    struct sigaction act;
    sigset_t mask;

    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = usr1_handler;

    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGUSR2);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    
    sigaction(SIGUSR1, &act, NULL);

    act.sa_sigaction = usr2_handler;
    sigaction(SIGUSR2, &act, NULL);


    printf("%d\n", getpid());
    while(1){
        sleep(1);
    }

    return 0;
}