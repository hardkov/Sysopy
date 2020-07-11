#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

pthread_t* client_queue;
int total_client_count;
int first_client;
int client_count;
int barber_seat;
int seats_count;
int shaved_count;

void* barber(void* arg){
    while(shaved_count != total_client_count){
        pthread_mutex_lock(&mutex);

        while(client_count == 0){
            barber_seat = -1;
            printf("Golibroda: ide spac\n");
            pthread_cond_wait(&cond, &mutex);
        }

        barber_seat = first_client;
        client_count--;
        printf("Golibroda: czeka %d klientów, gole klienta %lu\n", client_count, client_queue[first_client]);
        sleep((rand() % 3) + 3);
        
        first_client++;
        first_client %= seats_count;

        shaved_count++;
        
        pthread_mutex_unlock(&mutex);
    }

    return NULL;       
}

void* client(void* arg){
    int seat_number;
    pthread_t this_tid;
    bool TRY_AGAIN;

    this_tid = pthread_self();
    TRY_AGAIN = true;

    while(TRY_AGAIN){
        pthread_mutex_lock(&mutex);

        if(barber_seat == -1){
            client_queue[first_client] = this_tid;
            printf("Budze golibrode, %lu\n", this_tid);
            client_count++;
            barber_seat = -2; // client is heading to the barber seat so he is in superposition 
            pthread_cond_broadcast(&cond);
            TRY_AGAIN = false;
        }
        else if(client_count == seats_count){
            printf("Zajete, %lu\n", this_tid);
            pthread_mutex_unlock(&mutex);  
            sleep((rand() % 3) + 2); // trying again after several seconds
             
        }
        else{
            seat_number = (client_count + first_client) % seats_count;
            client_queue[seat_number] = this_tid;
            printf("Poczekalnia, wolne miejsca: %d, %lu\n", seats_count-client_count, this_tid);
            client_count++;
            
            TRY_AGAIN = false;
        }

    }

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char* argv[]){
    if(argc != 3){
        printf("Usage: %s seats_count total_client_count\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int i;
    pthread_t barber_tid;
    pthread_t *all_clients_tids;
    
    seats_count = atoi(argv[1]);
    total_client_count = atoi(argv[2]);

    barber_seat = -1;
    first_client = 0;
    client_count = 0;
    shaved_count = 0;
    client_queue = (pthread_t*) calloc(seats_count, sizeof(pthread_t));
    all_clients_tids = (pthread_t*) calloc(total_client_count, sizeof(pthread_t));
    
    srand(time(NULL));

    pthread_create(&barber_tid, NULL, barber, NULL);
 
    for(i = 0; i < total_client_count; i++){
        pthread_create(all_clients_tids+i, NULL, client, NULL);
        sleep((rand() % 3) + 1);
    }

    for(i = 0; i < total_client_count; i++){
        pthread_join(all_clients_tids[i], NULL);
    }

    pthread_join(barber_tid, NULL);

    free(all_clients_tids);
    free(client_queue);

    return 0;
}