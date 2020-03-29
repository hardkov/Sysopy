#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

void sgf_handler(int sig, siginfo_t* info, void* ucontext){
    printf("SIGSEGV captured:\nProblematic memory location:%p\n\n", info -> si_addr);
    printf("SIGSEGV was provoked by accessing a restricted area of memory\n");   
    exit(0);
}

void chld_handler(int sig, siginfo_t* info, void* ucontext){
    printf("SIGCHLD captured:\nUser id: %d\nExit value: %d\nUser time: %ld\nSystem time: %ld\n\n", info -> si_uid, info -> si_status,
    info -> si_utime, info -> si_stime);
    printf("SIGCHLD was provoked by creating child process and ending it\n");
    exit(0);
}

void fpe_handler(int sig, siginfo_t* info, void* ucontext){
    printf("SIGFPE captured:\nProblematic memory location:%p\n\n", info -> si_addr);
    printf("SIGFPE was provoked by dividing by zero\n");
    exit(0);
}
int main(int argc, char* argv[]){
    if(argc < 2){
        printf("usage: ./main sig\n");
        exit(0);
    }
    
    int sig;

    sig = atoi(argv[1]);

    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);

    act.sa_sigaction = sgf_handler;
    sigaction(SIGSEGV, &act, NULL);  // installing signal handlers
    
    act.sa_sigaction = chld_handler;
    sigaction(SIGCHLD, &act, NULL); 

    act.sa_sigaction = fpe_handler;
    sigaction(SIGFPE, &act, NULL);  

    if(sig == 11){
        char *s = "test string";  // provoke segfault
        *s = 'A';
    }
    else if(sig == 17){
        if(fork() == 0){
            while(sig < 2000000000) sig++;           // make babies
            return 1;
        }
        wait(NULL);
    }
    else if(sig == 8){
        sig = 0;
        sig = 1/sig;     // divide by 0
    }
    
    return 0;
}

