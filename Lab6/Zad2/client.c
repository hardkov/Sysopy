#define _POSIX_C_SOURCE 200809L
#include "my_conf.h"
#include <mqueue.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>

mqd_t server_qid;
mqd_t this_qid;
int this_id;
char* this_name;
int WORK_MODE; // 0 - requests, 1 - chating, 2 - finishing 
char* client_name;
int client_qid;
int client_id;

void init_request(char* queue_buf){
    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = INIT;
    strcpy(queue_buf+1, this_name);

    if(mq_send(server_qid, queue_buf, MSG_SIZE, 0) == -1){
        perror("blad");
    }

    memset(queue_buf, 0, MSG_SIZE);

    if(mq_receive(this_qid, queue_buf, MSG_SIZE, 0) == -1){
        perror("blad");
    };

    this_id = atoi(queue_buf+1);

    return;
}

void stop_request(char* queue_buf){
    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = STOP;
    sprintf(queue_buf+1, "%d", this_id);

    mq_send(server_qid, queue_buf, MSG_SIZE, 0);

    return;
}

void list_request(char* queue_buf){
    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = LIST;
    sprintf(queue_buf+1, "%d", this_id);

    mq_send(server_qid, queue_buf, MSG_SIZE, 0);

    memset(queue_buf, 0, MSG_SIZE);

    mq_receive(this_qid, queue_buf, MSG_SIZE, NULL);

    printf("%s", queue_buf+1);

    return;
}

void connect_request(char* queue_buf){
    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = CONNECT;
    sprintf(queue_buf+1, "%d %d",this_id, client_id);

    mq_send(server_qid, queue_buf, MSG_SIZE, 0);

    memset(queue_buf, 0, MSG_SIZE);

    mq_receive(this_qid, queue_buf, MSG_SIZE, NULL);
    
    strcpy(client_name, queue_buf+1);
    client_qid = mq_open(client_name, O_NONBLOCK | O_WRONLY);
    
    return;
}

void disconnect_request(char* queue_buf){
    memset(queue_buf, 0, MSG_SIZE);
    queue_buf[0] = DISCONNECT;
    sprintf(queue_buf+1, "%d %d", this_id, client_id);
    mq_send(server_qid, queue_buf, MSG_SIZE, 0);

    client_qid = -1;
    client_id = -1;
    memset(client_name, 0, MSG_SIZE);

    mq_close(client_qid);

    return;
}

void msg_request(char* queue_buf){
    memset(queue_buf, 0, BUF_SIZE);
    queue_buf[0] = MSG;
    scanf("%s", queue_buf+1);
    mq_send(client_qid, queue_buf, MSG_SIZE, 0); //MSG type
    return;
}

void handle_disconnect(char* queue_buf){
    mq_close(client_qid);

    client_qid = -1;
    client_id = -1;
    memset(client_name, 0, MSG_SIZE);

    return;
}

void handle_connect(char* queue_buf){
    strcpy(client_name, queue_buf+1);
    client_qid = mq_open(client_name, O_NONBLOCK | O_WRONLY);
    WORK_MODE = 1;

    return;
}

void handle_close(char* queue_buf){
    mq_close(server_qid);
    mq_close(this_qid); 
    mq_close(client_qid);
    mq_unlink(this_name);

    exit(0);
}

void sigusr1_handler(int sig_no){
    char* queue_buf = (char*) calloc(MSG_SIZE, sizeof(char));
    mq_receive(this_qid, queue_buf, MSG_SIZE, NULL);

    if(queue_buf[0] == CONNECT){
        handle_connect(queue_buf);
    }
    else if(queue_buf[0] == DISCONNECT){
        WORK_MODE = 0;
        handle_disconnect(queue_buf);
    }
    else if(queue_buf[0] == MSG){
        printf("%s\n", queue_buf+1);
    }
    else if(queue_buf[0] == CLOSE){
        handle_close(queue_buf);
    }
    else{
        printf("%s\n", queue_buf);
        printf("Otrzymano niespodziewany typ sygnału\n");
    }

    struct sigevent* sige = (struct sigevent*) calloc(1, sizeof(struct sigevent));
    sige -> sigev_notify = SIGEV_SIGNAL;
    sige -> sigev_signo = SIGUSR1;
    mq_notify(this_qid, sige);

    signal(SIGUSR1, sigusr1_handler);

    free(sige);
    free(queue_buf);
}

void sigint_handler(int sig_no){
    close(client_qid);
    close(this_qid);
    close(server_qid);
    mq_unlink(this_name);
    exit(0);
}

int main(int argc, char* argv[]){
    char* queue_buf = (char*) calloc(MSG_SIZE, sizeof(char));
    char* base = "/client";
    struct mq_attr* attr = (struct mq_attr*) calloc(1, sizeof(struct mq_attr));
    WORK_MODE = 0;
    attr -> mq_flags = 0;
    attr -> mq_maxmsg = 10;
    attr -> mq_msgsize = MSG_SIZE;
    attr -> mq_curmsgs = 0;
    this_name = (char*) calloc(MSG_SIZE, sizeof(char));
    sprintf(this_name, "%s%s", base, argv[1]);
    
    client_id = -1;
    client_name = (char*) calloc(MSG_SIZE, sizeof(char));
    client_qid = -1;

    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    
    this_qid = mq_open(this_name, O_RDONLY | O_CREAT, 0644, attr);    
    server_qid = mq_open("/server", O_WRONLY);

    if(this_qid == -1){
        perror("blad:");
    }

    if(server_qid == -1){
        perror("blad");
    } 
    
    struct sigevent* sige = (struct sigevent*) calloc(1, sizeof(struct sigevent));

    signal(SIGUSR1, sigusr1_handler);

    init_request(queue_buf);
    
    sige -> sigev_notify = SIGEV_SIGNAL;
    sige -> sigev_signo = SIGUSR1;
    mq_notify(this_qid, sige);

    while(WORK_MODE != 2){
        memset(buf, 0, BUF_SIZE);
        if(WORK_MODE == 0){
            scanf("%s", buf);

            if(strcmp(buf, "list") == 0){
                list_request(queue_buf);
            }
            else if(strcmp(buf, "stop") == 0){
                stop_request(queue_buf);
                WORK_MODE = 2;
            }
            else if(strcmp(buf, "connect") == 0){
                WORK_MODE = 1;
                scanf("%d", &client_id);
                connect_request(queue_buf);
            }
        }
        else if(WORK_MODE == 1){
            scanf("%s", buf);

            if(strcmp(buf, "disconnect") == 0){
                WORK_MODE = 0;
                disconnect_request(queue_buf);    
            }
            else if(strcmp(buf, "send") == 0){
                msg_request(queue_buf);
            }
        }
    }

    handle_close(queue_buf);

    free(queue_buf);
    free(attr);
    free(this_name);
    free(buf);  
    free(sige);

    return 0;
}