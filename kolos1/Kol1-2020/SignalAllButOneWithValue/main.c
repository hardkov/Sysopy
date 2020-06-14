#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


void sighandler(int sig_no, siginfo_t *siginfo, void *context){
    printf("Otrzymałem od rodzica: %d\n", siginfo -> si_value.sival_int);
}

int main(int argc, char* argv[]) {

    if(argc != 3){
        printf("Not a suitable number of program parameters\n");
        return 1;
    }

    struct sigaction action;
    action.sa_sigaction = &sighandler;

    //..........
    // po wykonainiu fork proces potomny ma taką samą obsługe sygnałów

    int child = fork();
    if(child == 0) {
        //zablokuj wszystkie sygnaly za wyjatkiem SIGUSR1
        //zdefiniuj obsluge SIGUSR1 w taki sposob zeby proces potomny wydrukowal
        //na konsole przekazana przez rodzica wraz z sygnalem SIGUSR1 wartosc
        sigset_t mask;
    
        sigfillset(&mask);
        sigdelset(&mask, SIGUSR1);
        sigprocmask(SIG_SETMASK, &mask, NULL);
        action.sa_mask = mask;
        action.sa_flags = SA_SIGINFO;

        sigaction(SIGUSR1, &action, NULL);

        pause(); // czekam na sygnał
    }
    else {
        //wyslij do procesu potomnego sygnal przekazany jako argv[2]
        //wraz z wartoscia przekazana jako argv[1]
        sleep(1);  // czekam chwile, żeby sygnał sie nie zgubił
        int int_val = atoi(argv[1]);
        __sigval_t val;
        val.sival_int = int_val;
        sigqueue(child, atoi(argv[2]), val);
    }

    return 0;
}
