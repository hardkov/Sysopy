#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


void dir_find(char* base_path){
    char* path = (char*) calloc(500, sizeof(char));
    char* buf = (char*) calloc(500, sizeof(char));
    struct dirent* fni = NULL;
    DIR* dir = opendir(base_path);

    if(dir == NULL) return;

    if(fork() == 0){
        printf("%d, %s\n", getpid(), base_path);
        sprintf(buf, "ls -l %s", base_path);
        system(buf);
        return;
    }

    wait(NULL);
    
    while((fni = readdir(dir)) != NULL){
        if(strcmp(fni -> d_name, ".") != 0 && strcmp(fni -> d_name, "..") != 0){
            strcpy(path, base_path);
            strcat(path, "/");
            strcat(path, fni -> d_name);

            dir_find(path);
        }
    }
    
    free(buf);
    free(path);
    closedir(dir);
}

int main(int argc, char* argv[]){
    if(argc == 2) dir_find(argv[1]);
    else dir_find(".");

    return 0;
}