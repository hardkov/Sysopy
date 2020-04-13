#define BUF_SIZE 200
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char* argv[]){
    if(argc != 2){
        printf("usage: ./main filename\n");
        exit(0);
    }

    char* filename = (char*) calloc(strlen(argv[1]), sizeof(char));
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char*));
    char* command = (char*) calloc(strlen(filename) + 5, sizeof(char));

    strcpy(filename, argv[1]);
    strcpy(command, "sort ");
    strcat(command, filename);

    FILE* sort_output = popen(command, "r");
    
    fread(buf, sizeof(char), BUF_SIZE, sort_output);
    
    printf("%s", buf);

    free(filename);
    free(buf);
    free(command);

    return 0;
}