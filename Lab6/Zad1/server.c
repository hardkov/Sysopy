#include "my_conf.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>


struct msgbuf {
    long mtype;       
    char mtext[MSG_SIZE];    
};

int AVAILABLE_ID;
int* clients_keys;
int* clients_msgqs;
bool* clients_open;
bool WORK;

int create_server_q(){
    return msgget(IPC_PRIVATE, IPC_CREAT | 00700);
}

void handle_init(struct msgbuf* mbuf){
    int key = atoi(mbuf -> mtext);
    int msgqid = msgget(key, 0);

    clients_keys[AVAILABLE_ID] = key;
    clients_msgqs[AVAILABLE_ID] = msgqid;

    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = INIT;
    sprintf(mbuf -> mtext, "%d", AVAILABLE_ID);

    msgsnd(msgqid, mbuf, MSG_SIZE, 0);
    
    AVAILABLE_ID++;

    return;
}

void handle_list(struct msgbuf* mbuf){
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    int i, msgqid;
    bool open;

    msgqid = clients_msgqs[atoi(mbuf -> mtext)];
    
    memset(mbuf -> mtext, 0, MSG_SIZE);
    for(i = 0; i < MAX_CLIENTS_NO; i++){
        if(clients_msgqs[i] != -1){
            memset(buf, 0, BUF_SIZE);
            open = clients_open[i];
            sprintf(buf, "id: %d, open: %s\n", i, open ? "true" : "false");
            strcat(mbuf->mtext, buf);
        }
    }

    mbuf -> mtype = LIST;
    msgsnd(msgqid, mbuf, MSG_SIZE, 0);

    free(buf);

    return;
}

void handle_disconnect(struct msgbuf* mbuf){
    clients_open[atoi(mbuf -> mtext)] = true;
    return;
}

void handle_stop(struct msgbuf* mbuf){
    int client_id = atoi(mbuf -> mtext);
    clients_keys[client_id] = -1;
    clients_msgqs[client_id] = -1;
    clients_open[client_id] = true;
    return;
}

void handle_connect(struct msgbuf* mbuf){
    char* msg_cpy = (char*) calloc(strlen(mbuf -> mtext), sizeof(char));
    char* token;

    strcpy(msg_cpy, mbuf -> mtext);

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
        memset(mbuf -> mtext, 0, MSG_SIZE);
        mbuf -> mtype = CONNECT;
        sprintf(mbuf -> mtext, "%d", clients_keys[client_id2]);

        msgsnd(clients_msgqs[client_id1], mbuf, MSG_SIZE, 0);
        
        memset(mbuf -> mtext, 0, MSG_SIZE);
        mbuf -> mtype = CONNECT;
        sprintf(mbuf -> mtext, "%d", clients_keys[client_id1]);

        msgsnd(clients_msgqs[client_id2], mbuf, MSG_SIZE, 0);

        clients_open[client_id1] = false;
        clients_open[client_id2] = false;

        return;
    }
}

void sigint_handler(int signum){
    WORK = false;
}

int main(int argc, char* argv[]){  
    int server_msqid, i;
    struct msgbuf* mbuf;

    signal(SIGINT, sigint_handler);
    
    clients_keys = (int*) calloc(MAX_CLIENTS_NO, sizeof(int));
    clients_msgqs = (int*) calloc(MAX_CLIENTS_NO, sizeof(int));
    clients_open = (bool*) calloc(MAX_CLIENTS_NO, sizeof(bool));
    mbuf = (struct msgbuf*) calloc(1, sizeof(struct msgbuf));
    AVAILABLE_ID = 0;
    WORK = true;

    server_msqid = create_server_q();

    for(i = 0; i < MAX_CLIENTS_NO; i++){
        clients_keys[i] = -1;
        clients_msgqs[i] = -1;
        clients_open[i] = true;
    }

    while(WORK){   // escaping via SIGINT
        msgrcv(server_msqid, mbuf, MSG_SIZE, -5, 0); 
        
        if(mbuf -> mtype == INIT) handle_init(mbuf);
        else if(mbuf -> mtype == LIST) handle_list(mbuf);
        else if(mbuf -> mtype == CONNECT) handle_connect(mbuf);
        else if(mbuf -> mtype == DISCONNECT) handle_disconnect(mbuf);
        else if(mbuf -> mtype == STOP) handle_stop(mbuf);

        memset(mbuf -> mtext, 0, MSG_SIZE);
    }

    msgctl(server_msqid, IPC_RMID, NULL);

    free(clients_keys);
    free(clients_msgqs);
    free(clients_open);
    free(mbuf);

    return 0;
}