#include "my_conf.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>


struct msgbuf {
    long mtype;       
    char mtext[MSG_SIZE];    
};

key_t unique_key;
int this_client_msgqid;
int server_msqid;
int this_client_id;
int WORK_MODE; // 0 - requests, 1 - chatting, 2 - finishing

void init_request(struct msgbuf* mbuf){
    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = INIT;
    sprintf(mbuf -> mtext, "%d", unique_key);

    msgsnd(server_msqid, mbuf, MSG_SIZE, 0);

    memset(mbuf -> mtext, 0, MSG_SIZE);
    msgrcv(this_client_msgqid, mbuf, MSG_SIZE, INIT, 0);

    this_client_id = atoi(mbuf -> mtext);

    return;
}

void list_request(struct msgbuf* mbuf){
    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = LIST;
    sprintf(mbuf -> mtext, "%d", this_client_id); 

    msgsnd(server_msqid, mbuf, MSG_SIZE, 0);

    memset(mbuf -> mtext, 0, MSG_SIZE);
    msgrcv(this_client_msgqid, mbuf, MSG_SIZE, LIST, 0);

    printf("%s", mbuf -> mtext);

    return;
}

int connect_request(struct msgbuf* mbuf, int client_id){
    int client_msgqid, client_key;

    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = CONNECT;
    sprintf(mbuf -> mtext, "%d %d", this_client_id, client_id); 
    msgsnd(server_msqid, mbuf, MSG_SIZE, 0);


    memset(mbuf -> mtext, 0, MSG_SIZE);
    msgrcv(this_client_msgqid, mbuf, MSG_SIZE, CONNECT, 0);
    client_key = atoi(mbuf -> mtext);
    client_msgqid = msgget(client_key, 0);

    WORK_MODE = 1;

    return client_msgqid;
}

void disconnect_request(struct msgbuf* mbuf){
    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = DISCONNECT;
    sprintf(mbuf -> mtext, "%d", this_client_id); 
    msgsnd(server_msqid, mbuf, MSG_SIZE, 0);

    WORK_MODE = 0;

    return;
}

void stop_request(struct msgbuf* mbuf){
    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = STOP;
    sprintf(mbuf -> mtext, "%d", this_client_id); 

    msgsnd(server_msqid, mbuf, MSG_SIZE, 0);

    WORK_MODE = 2;

    return;
}

int handle_connect(struct msgbuf* mbuf){
    WORK_MODE = 1;

    return msgget(atoi(mbuf -> mtext), 0);
}

void send_msg(struct msgbuf* mbuf, char* buf, int client_msgqid){
    memset(mbuf -> mtext, 0, MSG_SIZE);
    mbuf -> mtype = MSG;
    strcpy(mbuf -> mtext, buf);

    msgsnd(client_msgqid, mbuf, MSG_SIZE, 0);

    return;
}

void recieve_msg(struct msgbuf* mbuf){
    memset(mbuf -> mtext, 0, MSG_SIZE);

    while(msgrcv(this_client_msgqid, mbuf, MSG_SIZE, MSG, IPC_NOWAIT) != -1){
        printf("%s\n", mbuf -> mtext);
        memset(mbuf -> mtext, 0, MSG_SIZE);
    } 

    return;
}

void sigint_handler(int signum){
    WORK_MODE = 2;
}

int main(int argc, char* argv[]){
    struct msgbuf* mbuf;
    char* buf;
    int client_msgqid;
    int client_id;

    signal(SIGINT, sigint_handler);

    WORK_MODE = 0;
    mbuf = (struct msgbuf*) calloc(1, sizeof(struct msgbuf));
    buf = (char*) calloc(BUF_SIZE, sizeof(char));
    server_msqid = atoi(argv[1]);
    unique_key = ftok("./client", atoi(argv[2]));
    this_client_msgqid = msgget(unique_key, IPC_CREAT | 00700);

    init_request(mbuf);

    while(WORK_MODE != 2){  // escaped via SIGINT or stop request
        memset(buf, 0, BUF_SIZE);
        
        if(WORK_MODE == 0){ // reading requests
            if(msgrcv(this_client_msgqid, mbuf, MSG_SIZE, CONNECT, IPC_NOWAIT) != -1) client_msgqid = handle_connect(mbuf);
            scanf("%s", buf);
    
            if(strcmp(buf, "list") == 0) list_request(mbuf);
            else if(strcmp(buf, "connect") == 0){
                scanf("%d", &client_id);
                client_msgqid = connect_request(mbuf, client_id);
            }
            else if(strcmp(buf, "stop") == 0) stop_request(mbuf);
        }
        else if(WORK_MODE == 1){
            scanf("%s", buf);

            recieve_msg(mbuf);
            
            if(strcmp(buf, "msg") == 0){
                memset(buf, 0, BUF_SIZE);
                scanf("%s", buf);
                send_msg(mbuf, buf, client_msgqid);
            }
            else if(strcmp(buf, "disconnect") == 0) disconnect_request(mbuf);
        }
    }

    stop_request(mbuf);
    
    msgctl(this_client_msgqid, IPC_RMID, NULL);

    free(mbuf);
    free(buf);

    return 0;
}