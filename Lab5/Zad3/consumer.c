#define MAX_LINE_LEN 50
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]){
    int N, pipe;
    int out_file;
    char* pipe_path = (char*) calloc(strlen(argv[1]), sizeof(char));
    char* file_name = (char*) calloc(strlen(argv[2]), sizeof(char));
    char* buf;

    strcpy(pipe_path, argv[1]);
    strcpy(file_name, argv[2]);
    N = atoi(argv[3]);

    pipe = open(pipe_path, O_RDONLY);
    out_file = open(file_name, O_WRONLY);
    buf = (char*) calloc(N, sizeof(char));

    while(read(pipe, buf, N) > 0){
        write(out_file, buf, N);
        //sleep(1);
    }

    close(pipe);
    close(out_file);

    free(buf);
    free(pipe_path);
    free(file_name);

    return 0;
}