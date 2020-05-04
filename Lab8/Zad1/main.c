#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "pgma_io.h"
#include <pthread.h>
#include <sys/time.h>
#include <string.h>


int *histogram;
int *g;  
int x, y, gmax;
int thread_count;


long micro_timestamp(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}


static void* block(void* arg){
    int k, quot, start, end;
    int x_itr, y_itr, i;
    long *t = (long*) calloc(1, sizeof(long));

    *t = micro_timestamp();


    k = *(int*) arg;
    
    quot = (int) ceil(((float)(x))/((float)thread_count));
    start = (k-1) * quot;
    end = (k * quot) - 1;
    if(k == thread_count) end = x-1;

    for(x_itr = start; x_itr < end + 1; x_itr++){
        for(y_itr = 0; y_itr < y; y_itr++){
            i = x_itr + (y_itr * x);

            if(g[i] <= gmax){
                histogram[g[i]]++;
            }   
            else{
                printf("Data element size error");
                exit(EXIT_FAILURE);
            }
        }
    }

    *t -= micro_timestamp();

    return  (void*) t;
}

static void* sign(void* arg){
    int k, quot, start, end, i;
    long *t = (long*) calloc(1, sizeof(long));

    *t = micro_timestamp();
    
    k = *(int*) arg;
    
    quot = (int) ceil(((float)(gmax+1))/((float)thread_count));
    start = (k-1) * quot;
    end = (k * quot) - 1;
    if(k == thread_count) end = gmax;

    for(i = 0; i < x * y; i++){
        if(g[i] <= gmax && g[i] >= start && g[i] <= end){
            histogram[g[i]]++;
        }   
        else if(g[i] > gmax){
            printf("Data element size error");
            exit(EXIT_FAILURE);
        }
    }

    *t -= micro_timestamp();

    return  (void*) t;
}

static void* interleaved(void* arg){
    int x_itr, y_itr, i, k;
    long *t = (long*) calloc(1, sizeof(long));

    *t = micro_timestamp();

    k = *(int*) arg;

    for(x_itr = k-1; x_itr < x; x_itr += thread_count){
        for(y_itr = 0; y_itr < y; y_itr++){
            i = x_itr + (y_itr * x);

            if(g[i] <= gmax){
                histogram[g[i]]++;
            }   
            else{
                printf("Data element size error");
                exit(EXIT_FAILURE);
            }
        }
    }

    *t -= micro_timestamp();

    return  (void*) t;
}

void save_times(char *mode, long* times, long total_time, pthread_t* tids){
    int i;
    FILE* f;

    f = fopen("hist_times", "a+");
    if(f == NULL){
        printf("Can't open file to save times\n");
        exit(EXIT_FAILURE);
    }

    fprintf(f, "MODE: %s, TID: %lu, TOTAL TIME: %ld, THREAD COUNT: %d\n", mode, pthread_self(), total_time, thread_count);

    for(i = 0; i < thread_count; i++){
        fprintf(f, "TID: %lu, TIME: %ld\n", tids[i], times[i]);
    }

    fprintf(f, "\n");

    return;
}

void save_histogram(char* mode, char* filename){
    int i;
    FILE* f;

    f = fopen(filename, "a+");
    if(f == NULL){
        printf("Can't open file to save histogram\n");
        exit(EXIT_FAILURE);
    }

    fprintf(f, "MODE: %s\n", mode);

    for(i = 0; i < gmax+1; i++){
        fprintf(f, "COLOR: %d, COUNT: %d\n", i, histogram[i]);
    }

    fprintf(f, "\n");

    return;
}

int main(int argc, char* argv[]){
    if(argc != 5){
        printf("Usage: %s thread_count mode in_filename out_filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* in_filename;
    char* out_filename;
    char* mode;
    pthread_t* tids;
    void* start_function, *rval_ptr; 
    int i, *args;
    long total_time, *times;

    thread_count = atoi(argv[1]);
    mode = argv[2];
    in_filename =  argv[3];
    out_filename =  argv[4];

    if(strcmp(mode, "interleaved") == 0) start_function = interleaved;
    else if(strcmp(mode, "block") == 0) start_function = block;
    else if(strcmp(mode, "sign") == 0) start_function = sign;
    else{
        printf("Unknown mode\n");
        exit(EXIT_FAILURE);
    }

    pgma_read(in_filename, &x, &y, &gmax, &g);
    
    tids = (pthread_t*) calloc(thread_count, sizeof(pthread_t));
    args = (int*) calloc(thread_count, sizeof(int));
    histogram = (int*) calloc(gmax+1, sizeof(int));
    times = (long*) calloc(thread_count, sizeof(long));

    total_time = micro_timestamp();

    for(i = 0; i < thread_count; i++){
        args[i] = i+1;
        pthread_create(tids+i, NULL, start_function,(void*) &args[i]);    
    }

    for(i = 0; i < thread_count; i++){
        pthread_join(*(tids+i), &rval_ptr);
        times[i] = *(long*) rval_ptr;
        times[i] *= -1;
        free(rval_ptr);
    }

    total_time -= micro_timestamp();
    total_time *= -1;

    save_times(mode, times, total_time, tids);
    save_histogram(mode, out_filename);

    free(times);
    free(args);
    free(g);
    free(histogram);
    free(tids);

    return 0;
}