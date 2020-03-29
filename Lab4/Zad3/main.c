#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig, siginfo_t* info, void* ucontext){
    printf("PID: %d SIG: %d UID: %d CODE: %d SYS TIME: %ld\n", info -> si_pid, sig, info -> si_uid,
    info -> si_code, info -> si_stime);   
}

int main(int argc, char* argv[]){
    int i;

    if(argc < 2) printf("usage: ./main <list of signal identificators(numbers), space-separated>\n");
    
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = handler;
    
    printf("%d\n", getpid());

    for(i = 1; i < argc; i++){
        sigaction(atoi(argv[i]), &act, NULL);  // installing signal handlers
    }
    while(1){
            printf("Working...\n");
            sleep(1);    
    }

    return 0;
}

