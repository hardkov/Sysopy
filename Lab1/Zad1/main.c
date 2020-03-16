#include "mylib.h"
#include <string.h>

int main(int argc, char* argv[]){
    
    char** files = (char**) calloc(argc-1, sizeof(char*));
    int i;
    for(i = 1; i < argc; i++){
        files[i-1] = argv[i];
    }

    struct block_container* bl_c = create_table((argc-1)/2);

    compare_files(files, argc-1);
    
    for(i = 0; i < (argc-1)/2; i++){
        char* tmp_file_name = (char*) calloc(10, sizeof(char));
        sprintf(tmp_file_name, "tmp_file%d", i);
        create_block(tmp_file_name, bl_c);
    }
    int j;
    for(i = 0; i < (argc-1)/2; i++){
        for(j = 0; j < 3; j++){
            printf("%s", bl_c -> blocks[i] -> operations[j]); 
        }
    }
}