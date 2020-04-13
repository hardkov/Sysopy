#define MAX_LINE_LEN 50
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


char* form_line(int N, int pid, char* buf, int* line_size){
    char* line = (char*) calloc(MAX_LINE_LEN, sizeof(char));
    
    sprintf(line, "#%d#%s\n", pid, buf);                                 // \n sign should not be in input file
    *line_size = strlen(line);
    
    return line;
}

int main(int argc, char* argv[]){

    int N, pid, pipe;
    int in_file, line_size;
    char* pipe_path = (char*) calloc(strlen(argv[1]), sizeof(char));
    char* file_name = (char*) calloc(strlen(argv[2]), sizeof(char));
    char* buf;
    char* line;

    strcpy(pipe_path, argv[1]);
    strcpy(file_name, argv[2]);
    N = atoi(argv[3]);

    pid = getpid();
    pipe = open(pipe_path, O_WRONLY);
    in_file = open(file_name, O_RDONLY);
    buf = (char*) calloc(N, sizeof(char));

    while(read(in_file, buf, N) > 0){
        line = form_line(N, pid, buf,  &line_size);
        write(pipe, line, line_size);
        free(line);
        sleep(1);
    }

    close(pipe);
    close(in_file);

    free(buf);
    free(pipe_path);
    free(file_name);

    return 0;
}