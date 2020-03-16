#ifndef MYLIB_H
#define MYLIB_H
#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct block{
    char** operations;
    int size;
    int ammount_of_operations;
};

struct block_container{
    struct block** blocks;
    int size;
};

void remove_operation(int bl_index, int index, struct block_container* bl_c);

void remove_block(struct block_container* bl_c, int index);

int create_block(char* file, struct block_container* bl_c);

void compare_files(char** files, int size);

struct block_container* create_table(int size);

int get_ammount_of_operations(int index, struct block_container* bl_c);

#endif
