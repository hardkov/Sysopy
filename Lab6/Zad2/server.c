#include "my_conf.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <mqueue.h>


int AVAILABLE_ID;
char** clients_names;
int* clients_qs;
bool* clients_open;
bool WORK;
mqd_t server_q;

void handle_init(char* queue_buf){
    strcpy(clients_names[AVAILABLE_ID], queue_buf+1);

    mqd_t qid = mq_open(queue_buf+1, O_WRONLY);
    
    if(qid == -1){
        perror("blad");
    }
    clients_qs[AVAILABLE_ID] = qid;

    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = INIT;
    sprintf(queue_buf+1, "%d", AVAILABLE_ID);

    mq_send(qid, queue_buf, MSG_SIZE, 0);

    AVAILABLE_ID++;

    return;
}

void handle_list(char* queue_buf){
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    int i, qid;
    bool open;

    qid = clients_qs[atoi(queue_buf+1)];
    
    memset(queue_buf, 0, MSG_SIZE);
    for(i = 0; i < MAX_CLIENTS_NO; i++){
        if(clients_qs[i] != -1){
            memset(buf, 0, BUF_SIZE);
            open = clients_open[i];
            sprintf(buf, "id: %d, open: %s\n", i, open ? "true" : "false");
            strcat(queue_buf+1, buf);
        }
    }

    mq_send(qid, queue_buf, MSG_SIZE, 0);

    free(buf);

    return;
}

void handle_disconnect(char* queue_buf){
    char* msg_cpy = (char*) calloc(strlen(queue_buf+1), sizeof(char));
    char* token;

    strcpy(msg_cpy, queue_buf+1);

    token = strtok(msg_cpy, " ");
    int client_id1 = atoi(token);

    token = strtok(NULL, " ");
    int client_id2 = atoi(token);


    clients_open[client_id1] = true;
    clients_open[client_id2] = true;

    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = DISCONNECT;
    mq_send(clients_qs[client_id2], queue_buf, MSG_SIZE, 0);

    return;
}

void handle_stop(char* queue_buf){
    int client_id = atoi(queue_buf+1);

    mq_close(clients_qs[client_id]);

    clients_qs[client_id] = -1;
    clients_open[client_id] = true;

    return;
}

void handle_connect(char* queue_buf){
    char* msg_cpy = (char*) calloc(strlen(queue_buf+1), sizeof(char));
    char* token;

    strcpy(msg_cpy, queue_buf+1);

    token = strtok(msg_cpy, " ");
    int client_id1 = atoi(token);

    token = strtok(NULL, " ");
    int client_id2 = atoi(token);


    if(clients_open[client_id1] == false || clients_open[client_id2] == false){
        printf("Can't make a connection, one client is busy\n");
        return;
    } 
    else if(client_id1 == client_id2){
        printf("Can't make a connection between the same clients");
    }
    else{
        memset(queue_buf, 0, MSG_SIZE);
        queue_buf[0] = CONNECT;
        sprintf(queue_buf+1, "%s", clients_names[client_id2]);

        mq_send(clients_qs[client_id1], queue_buf, MSG_SIZE, 0);
        
        memset(queue_buf, 0, MSG_SIZE);
        queue_buf[0] = CONNECT;
        sprintf(queue_buf+1, "%s", clients_names[client_id1]);

        mq_send(clients_qs[client_id2], queue_buf, MSG_SIZE, 0);

        clients_open[client_id1] = false;
        clients_open[client_id2] = false;

        return;
    }
}

void close_requests(char* queue_buf){
    int i;
    for(i = 0; i < MAX_CLIENTS_NO; i++){
        if(clients_qs[i] != -1){
            memset(queue_buf, 0, MSG_SIZE);
            queue_buf[0] = CLOSE;
            mq_send(clients_qs[i], queue_buf, MSG_SIZE, 0);

            mq_close(clients_qs[i]);
        }   
    }

    return;
}

void sigint_handler(int signum){
    char* queue_buf = (char*) calloc(MSG_SIZE, sizeof(char));
    WORK = false;
    close_requests(queue_buf);
    
    mq_close(server_q);
    mq_unlink("/server");
    exit(0);
}

int main(int argc, char* argv[]){ 
    int i;
    char* queue_buf;
    struct mq_attr* attr;

    signal(SIGINT, sigint_handler);
    
    clients_names = (char**) calloc(MAX_CLIENTS_NO, sizeof(char*));
    clients_qs = (int*) calloc(MAX_CLIENTS_NO, sizeof(int));
    clients_open = (bool*) calloc(MAX_CLIENTS_NO, sizeof(bool));
    queue_buf = (char*) calloc(MSG_SIZE, sizeof(char));
    AVAILABLE_ID = 0;
    WORK = true;

    attr = (struct mq_attr*) calloc(1, sizeof(struct mq_attr));
    attr -> mq_flags = 0;
    attr -> mq_maxmsg = 10;
    attr -> mq_msgsize = MSG_SIZE;
    attr -> mq_curmsgs = 0;

    server_q = mq_open("/server", O_CREAT | O_RDONLY, 0644, attr); 
    if(server_q == -1){
        perror("blad");
    }

    for(i = 0; i < MAX_CLIENTS_NO; i++){
        clients_qs[i] = -1;
        clients_open[i] = true;
        clients_names[i] = (char*) calloc(MSG_SIZE, sizeof(char));
    }

    while(WORK){   // escaping via SIGINT
     
        if(mq_receive(server_q, queue_buf, MSG_SIZE, NULL) == -1){
            perror("blad");
        } 

        if(queue_buf[0] == INIT){
            printf("Otrzymano INIT\n");
            handle_init(queue_buf);
        } 
        else if(queue_buf[0] == LIST){
            printf("Otrzymano LIST\n");
            handle_list(queue_buf);
        } 
        else if(queue_buf[0] == CONNECT){
            printf("Otrzymano CONNECT\n");
            handle_connect(queue_buf);
        } 
        else if(queue_buf[0] == DISCONNECT){
            printf("Otrzymano DISCONNECT\n");
            handle_disconnect(queue_buf);
        }
        else if(queue_buf[0] == STOP){
            printf("Otrzymano STOP\n");
            handle_stop(queue_buf);
        } 

        memset(queue_buf, 0, MSG_SIZE);
    }

    return 0;
}