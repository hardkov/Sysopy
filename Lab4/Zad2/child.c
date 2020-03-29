#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]){
    int sig = atoi(argv[2]);

    if(strcmp(argv[1], "ignore") == 0){
        raise(sig);
        printf("Exec dziedziczy ignorowanie sygnalu: %d\n", sig);
    }
    else if(strcmp(argv[1], "mask") == 0){
        sigset_t actual_mask;
        raise(sig);
        sigpending(&actual_mask);
        if(sigismember(&actual_mask, sig) == 1) printf("Sygnal jest maskowany po wykonaniu exec: %d\n", sig);
    }
    else if(strcmp(argv[1], "pending") == 0){
        sigset_t actual_mask;
        sigpending(&actual_mask);
        if(sigismember(&actual_mask, sig) == 1) printf("Sygnal do rodzica oczekuje po wykonaniu exec: %d\n", sig);
    }

    return 0;
}