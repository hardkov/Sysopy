#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char* argv[]){
    mkfifo("./pipe", 0666);
    
    if(fork() == 0){
        execl("./prod", "./prod", "./pipe", "prod1.txt", "1", NULL);
        exit(0);
    }

    if(fork() == 0){
        execl("./prod", "./prod", "./pipe", "prod2.txt", "2", NULL);
        exit(0);
    }

    if(fork() == 0){
        execl("./prod", "./prod", "./pipe", "prod3.txt", "3", NULL);
        exit(0);
    }

    if(fork() == 0){
        execl("./prod", "./prod", "./pipe", "prod4.txt", "4", NULL);
        exit(0);
    }

    if(fork() == 0){
        execl("./prod", "./prod", "./pipe", "prod5.txt", "5", NULL);
        exit(0);
    }

    if(fork() == 0){
        execl("./cons", "./cons", "./pipe", "cons.txt", "10", NULL);
        exit(0);
    }

    return 0;
}