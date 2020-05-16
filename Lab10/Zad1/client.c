#define _DEFAULT_SOURCE _POSIX_C_SOURCE < 200809L
#define _BSD_SOURCE || _SVID_SOURCE
#define MAX_CLIENTS 10
#define BUF_SIZE 100
#define CONNECTED_MSG "Polaczono pomyslnie"
#define NAME_TAKEN_MSG "Nazwa zajeta!"
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <endian.h>
#include <string.h>


int main(int argc, char* argv[]){
    int i, sfd;
    char* text_addr = "0.0.0.0";
    char *buf = (char*) calloc(BUF_SIZE, sizeof(char));

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd == -1){
        perror("Socket error");
    }

    struct in_addr sin_addr;
    if(inet_aton(text_addr, &sin_addr) == 0){
        perror("Inet_aton error");
    } 

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(39999);
    addr.sin_addr = sin_addr;

    if(connect(sfd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
        perror("Connect error");
    }

    strcpy(buf, argv[1]);
    send(sfd, buf, BUF_SIZE, 0);
    perror("send");

    memset(buf, 0, BUF_SIZE);
    recv(sfd, buf, BUF_SIZE, 0);
    perror("recieve");

    printf("%s\n", buf);
    /*
    if(strcmp(buf, "Nazwa zajeta!") == 0){
        printf("Nazwa zajeta :(\n");
    }
    else{
        // będzie grane
        printf("Polaczylem sie!\n");
    }
    */

    scanf("%s", buf);

    if(close(sfd) == -1){
        perror("Close error");
    } 

    return 0;
}