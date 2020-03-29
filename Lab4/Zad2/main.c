#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>


void handler(int signum){
    printf("Otrzymalem sygnal: %d\n", signum);
}

int main(int argc, char* argv[]){
    if(argc != 3){
        printf("usage: ./main mode sig_no\n");
        exit(0);
    }

    int sig;
    char * mode = (char*) calloc(strlen(argv[1]), sizeof(char*));
    char* sig_string = (char*) calloc(10, sizeof(char));
    struct sigaction act;

    strcpy(mode, argv[1]);
    sig = atoi(argv[2]);

    sprintf(sig_string, "%d", sig);
    act.sa_flags = 0;
    act.sa_handler = SIG_DFL;
    sigemptyset(&act.sa_mask);
        
    if(strcmp(mode, "ignore") == 0){
        act.sa_handler = SIG_IGN;
        sigaction(sig, &act, NULL);
        raise(sig);

        if(fork() == 0){
            raise(sig);
            printf("Potomny proces dziedziczy ignorowanie sygnalu %d\n", sig);
            return 0;
        }
        wait(NULL);
        execl("./child", "child", mode, sig_string, NULL);
    }
    else if(strcmp(mode, "handler") == 0){
        act.sa_handler = handler;

        sigaction(sig, &act, NULL);

        raise(sig);
        if(fork() == 0){
            raise(sig);
            return 0;
        }
    }
    else if(strcmp(mode, "mask") == 0){
        sigset_t new_mask;
        sigset_t actual_mask;
        sigemptyset(&new_mask);
        sigaddset(&new_mask, sig);
        sigprocmask(SIG_SETMASK, &new_mask, NULL);
        
        raise(sig);

        if(fork() == 0){
            raise(sig);
            sigpending(&actual_mask);
            if(sigismember(&actual_mask, sig) == 1) printf("Sygnal jest maskowany w procesie potomnym: %d\n", sig);
            return 0;
        }
        wait(NULL);
        execlp("./child", "child", mode, sig_string, NULL);
    }
    else if(strcmp(mode, "pending") == 0){
        sigset_t new_mask;
        sigset_t actual_mask;
        sigemptyset(&new_mask);
        sigaddset(&new_mask, sig);
        sigprocmask(SIG_SETMASK, &new_mask, NULL);

        raise(sig);
        if(fork() == 0){
            sigpending(&actual_mask);
            if(sigismember(&actual_mask, sig) == 1) printf("Sygnal do rodzica oczekuje w procesie potomnym: %d\n", sig);
            return 0;
        }
        wait(NULL);
        execlp("./child", "child", mode, sig_string, NULL);
    }

    return 0;
}