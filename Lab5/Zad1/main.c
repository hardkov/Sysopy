#define BUF_SIZE 1000
#define SEQUENCE_LIMIT 100
#define PROGRAM_LIMIT 50
#define MAX_ARG_LEN 20
#define ARG_LIMIT 10
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

char** create_sequence_set(char* file, int* set_size){
    int i;
    char* buf;
    char** sequence_set;
    char* tmp_sequence;
    FILE* f;

    buf = (char*) calloc(BUF_SIZE, sizeof(char));
    sequence_set = (char**) calloc(SEQUENCE_LIMIT, sizeof(char*));
    for(i = 0; i < SEQUENCE_LIMIT; i++){
        sequence_set[i] = (char*) calloc(BUF_SIZE, sizeof(char));
    }

    f = fopen(file, "r");
    fread(buf, sizeof(char), BUF_SIZE, f);
    fclose(f);

    i = 0;
    tmp_sequence = strtok(buf, "\n");
    while(tmp_sequence != NULL){
        strcpy(sequence_set[i], tmp_sequence);
        tmp_sequence = strtok(NULL, "\n");
        i++;
    }

    *set_size = i;
    // can't free memory bcs of strtok
    return sequence_set;
}

char** create_program_set(char* sequence, int* size_buf){
    int i;
    char** program_set;
    char* tmp_program;
    char* cp_sequence;

    cp_sequence = (char*) calloc(strlen(sequence), sizeof(char));
    strcpy(cp_sequence, sequence);
    program_set = (char**) calloc(PROGRAM_LIMIT, sizeof(char*));
    for(i = 0; i < PROGRAM_LIMIT; i++){
        program_set[i] = (char*) calloc(BUF_SIZE, sizeof(char));
    }

    i = 0;
    tmp_program = strtok(cp_sequence, "|");
    while(tmp_program != NULL){
        strcpy(program_set[i], tmp_program);
        tmp_program = strtok(NULL, "|");
        i++;
    }
    // can't free memory bcs of strtok
    *size_buf = i;
    return program_set;
}

void exec_program(char* program){
    int i;
    char* cp_program = (char*) calloc(strlen(program), sizeof(char));
    char* tmp_arg;
    char** args = (char**) calloc(ARG_LIMIT, sizeof(char*));

    strcpy(cp_program, program);
    
    tmp_arg = strtok(cp_program, " ");
    i = 0;
    while(tmp_arg != NULL){
        args[i] = (char*) calloc(MAX_ARG_LEN, sizeof(char));
        strcpy(args[i], tmp_arg);
        tmp_arg = strtok(NULL, " ");
        i++;
    }

    for(i = i; i < ARG_LIMIT; i++){
        args[i] = NULL;
    }
    // can't free memory bcs of strtok
    execvp(args[0], args);
    exit(1);
}

void exec_program_set(char** program_set, int program_counter){
    int i;
    int curr[2];
    int prev[2];

    for(i = 0; i < program_counter; i++){
        pipe(curr);
        if(fork() == 0){
            if(i == 0){
                close(curr[0]);
                dup2(curr[1], STDOUT_FILENO);
                exec_program(program_set[i]);
            }
            else if(i == program_counter-1){
                close(curr[0]);
                close(curr[1]);
                close(prev[1]); // --same--
                dup2(prev[0], STDIN_FILENO);
                exec_program(program_set[i]);
            }
            else{
                close(prev[1]); // might be closing two times? 
                close(curr[0]);
                dup2(prev[0], STDIN_FILENO);
                dup2(curr[1], STDOUT_FILENO);
                exec_program(program_set[i]);
            }
        }
        close(curr[1]);
        prev[0] = curr[0];
        prev[1] = curr[1];
    }

    while(wait(NULL) > 0);
}

int main(int argc, char* argv[]){
    if(argc != 2){
        printf("usage: ./main file_name");
        exit(0);
    }

    int size, seq_size, i, j;
    char** program_set;
    char** sequence_set;
    char* file_name = (char*) calloc(strlen(argv[1]), sizeof(char));

    strcpy(file_name, argv[1]);
    sequence_set = create_sequence_set(file_name, &seq_size);

    for(i = 0; i < seq_size; i++){
        program_set = create_program_set(sequence_set[i], &size);
        exec_program_set(program_set, size);

        // free program set memory
        for(j = 0; j < PROGRAM_LIMIT; j++){
            free(program_set[j]);
        }
        free(program_set);
        // ---------------------------
    }

    // free sequence set memory
    for(i = 0; i < SEQUENCE_LIMIT; i++){
        free(sequence_set[i]);
    }
    free(sequence_set);
    // ------------------------------------

    free(file_name);
    return 0;
}