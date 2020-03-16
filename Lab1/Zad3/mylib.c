#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mylib.h"

void remove_operation(int bl_index, int index, struct block_container* bl_c){
    if(bl_index >= bl_c -> size){
        printf("Index %d out of bounds %d", bl_index, bl_c -> size);
        exit(0);
    }
    
    if(index >= bl_c -> blocks[bl_index] -> size){
        printf("Index %d out of bounds %d", index, bl_c -> blocks[bl_index] -> size);
        exit(0);
    }

    bl_c -> blocks[bl_index] -> ammount_of_operations--;
    free(bl_c -> blocks[bl_index] -> operations[index]);
    bl_c -> blocks[bl_index] -> operations[index] = NULL;
}

void remove_block(struct block_container* bl_c, int index){
    if(index >= bl_c -> size){
        printf("Index %d out of bounds %d", index, bl_c -> size);
        exit(0);
    }
    
    int i;
    for(i = 0; i < bl_c -> blocks[index] -> size; i++){
        remove_operation(index, i, bl_c);
    }

    free(bl_c -> blocks[index]);
    bl_c -> blocks[index] = NULL;
}

int create_block(char* file, struct block_container* bl_c){
    char** tmp = NULL;
    struct block* bl = (struct block*) calloc(1, sizeof(struct block));
    int new_block_index = -1; 
    char* buf = (char*) calloc(100100, sizeof(char));
    int size = 0;

    FILE* f = fopen(file, "r");
    if(f == NULL){
        exit(EXIT_FAILURE);
    }
    fread(buf, sizeof(char), 100000, f);
    fclose(f);

    int i = 0;
    while(buf[i] != '\0'){
        if(i == 0 || (buf[i-1] == '\n' && buf[i] >= 48 && buf[i] <= 57)) size++;
        i++;
    }
    tmp = (char**) calloc(size, sizeof(char*));
    int tmp_counter = -1;
    int buf_counter = 0;
    
    while(buf[buf_counter] != '\0'){
        if(buf_counter == 0 || (buf[buf_counter-1] == '\n' && buf[buf_counter] >= 48 && buf[buf_counter] <= 57)){
            tmp_counter++;
            tmp[tmp_counter] = (char*) calloc(10000, sizeof(char));
            strncat(tmp[tmp_counter], &buf[buf_counter], 1);
        }
        else{
            strncat(tmp[tmp_counter], &buf[buf_counter], 1);
        }
        buf_counter++;
    }
        
    bl -> ammount_of_operations = size;
    bl -> size = size;
    bl -> operations = tmp;

    for(i = 0; i < bl_c -> size; i++){
        if(bl_c -> blocks[i] == NULL){
            new_block_index = i;
            break;
        }         
    }

    if(new_block_index == -1){
        printf("No free space available in block container for new block");
        exit(0);
    }

    bl_c -> blocks[new_block_index] = bl;

    return new_block_index;
}

void compare_files(char** files, int size){
    if(size % 2 != 0){
        printf("Wrong number of files: %d", size);
        exit(0);
    }

    int i;
    for(i = 0; i < size; i += 2){
        char* diff_command = (char*) calloc(20 + strlen(files[i]) + strlen(files[i+1]), sizeof(char));
        char* tmp_file_name = (char*) calloc(10, sizeof(char));
        char* touch_command = (char*) calloc(strlen(tmp_file_name) + 6, sizeof(char));

        sprintf(tmp_file_name, "tmp_file%d", i/2);
        strcpy(touch_command, "touch ");
        strcat(touch_command, tmp_file_name);
        system(touch_command);
        
        strcpy(diff_command, "diff ");
        strcat(diff_command, files[i]);
        strcat(diff_command, " ");
        strcat(diff_command, files[i+1]);
        strcat(diff_command, " >> ");
        strcat(diff_command, tmp_file_name);
        system(diff_command);
    }
}

struct block_container* create_table(int size){
    struct block_container* bl_c = (struct block_container*) calloc(1, sizeof(struct block_container));
    
    bl_c -> size = size;
    bl_c -> blocks = (struct block**) calloc(size, sizeof(struct block*));
    int i;
    for(i = 0; i < size; i++){
        bl_c -> blocks[i] = NULL;
    }
    return bl_c; 
}

int get_ammount_of_operations(int index, struct block_container* bl_c){
    if(index >= bl_c -> size){
        printf("Index %d out of bounds %d ", index, bl_c -> size);
        return -1;
    }

    return bl_c -> blocks[index] -> ammount_of_operations;
}