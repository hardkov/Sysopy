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
#include <stdbool.h>
#include <pthread.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


int* clients;
int* connected_with;
char** clients_names;

int client_counter;
int fst_available_idx;

int waiting_player_idx;
int new_player_idx;

int sfd;

bool is_name_taken(char* name){
    int i;

    for(i=0; i<MAX_CLIENTS; i++){
        if(clients_names[i] != NULL && strcmp(clients_names[i], name) == 0){
            return true;
        }
    }

    return false;
}

void* listener(void* arg){
    int i;
    int cfd;
    char *buf = (char*) calloc(BUF_SIZE, sizeof(char));

    struct sockaddr_in addr;

    socklen_t addrlen = sizeof(addr);

    while(fst_available_idx != MAX_CLIENTS){
        cfd = accept(sfd, (struct sockaddr*) &addr, &addrlen);  

        if(cfd == -1){
            perror("Accept error");
        } 
        else{
            recv(cfd, buf, BUF_SIZE, 0); // cfd or sfd?

            if(is_name_taken(buf)){
                memset(buf, 0, BUF_SIZE);
                strcpy(buf, NAME_TAKEN_MSG);
                send(cfd, buf, BUF_SIZE, 0);

                close(cfd); // ok??
            }
            else{
                pthread_mutex_lock(&mutex);
                
                clients[fst_available_idx] = cfd;
                strcpy(clients_names[fst_available_idx], buf);

                client_counter++;
                fst_available_idx++;

                pthread_mutex_unlock(&mutex);

                memset(buf, 0, BUF_SIZE);
                strcpy(buf, CONNECTED_MSG);
                send(cfd, buf, BUF_SIZE, 0);
            }
        }
    }

    return NULL;
}

void* game_handler(void* arg);

int main(int argc, char* argv[]){
    int i;
    char *text_addr;
    pthread_t listen_tid;

    clients = (int*) calloc(MAX_CLIENTS, sizeof(int));
    connected_with = (int*) calloc(MAX_CLIENTS, sizeof(int));;
    clients_names = (char**) calloc(MAX_CLIENTS, sizeof(char*));;
    client_counter = 0;
    fst_available_idx = 0;
    new_player_idx = -1;
    waiting_player_idx = -1;

    for(i=0; i<MAX_CLIENTS; i++){
        clients[i] = -1;
        connected_with[i] = -1;
        clients_names[i] = (char*) calloc(BUF_SIZE, sizeof(char));
    }

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd == -1){
        perror("Socket creation");
    }

    struct in_addr sin_addr;
    sin_addr.s_addr = INADDR_ANY; 

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(39999);
    addr.sin_addr = sin_addr;

    socklen_t addrlen = sizeof(addr);

    if(bind(sfd, (struct sockaddr*) &addr, addrlen) == -1){ // 
        perror("Socked binding");
    }

    if(getsockname(sfd, (struct sockaddr*) &addr, &addrlen) == -1){
        perror("Getsockname error");
    }

    text_addr = inet_ntoa(addr.sin_addr);
    printf("Moj adres: %s\n", text_addr);

    if(listen(sfd, MAX_CLIENTS) == -1){
        perror("Listen error");
    }

    // accepting thread
    pthread_create(&listen_tid, NULL, listener, NULL);


    //////////////////////

    pthread_join(listen_tid, NULL);

    //////////////////////

    if(close(sfd) == -1){
        perror("Close error");
    } 

    
    free(clients);

    for(i=0; i<MAX_CLIENTS; i++) free(clients_names[i]);
    free(clients_names);
    
    free(connected_with);

    

    return 0;
}