#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/times.h>

void generate(char* file_name, int number_of_records, int size){
    char* head_command = (char*) calloc(100, sizeof(char));
    sprintf(head_command, "head -c %d /dev/urandom > %s", number_of_records * size, file_name);
    system(head_command);

    free(head_command);
} 

void sys_copy(int sf, int df, int number_of_records, int size){
    char* buf = (char*) calloc(size, sizeof(char));

    int i;
    for(i = 0; i < number_of_records; i++){
        if(read(sf, buf, size) == size){
            write(df, buf, size);
        }
    }

    free(buf);
}

void lib_copy(FILE* sf, FILE* df, int number_of_records, int size){
    char* buf = (char*) calloc(size, sizeof(char));

    int i;
    for(i = 0; i < number_of_records; i++){
        if(fread(buf, sizeof(char), size, sf) == size){
            fwrite(buf, sizeof(char), size, df);    
        }
    }

    free(buf);
}

void sys_swap(int f, int a, int b, int size){
    char* buf = (char*) calloc(size, sizeof(char));
    char* tmp = (char*) calloc(size, sizeof(char));
    
    lseek(f, a*size, 0);
    read(f, buf, size);

    lseek(f, b*size, 0);
    read(f, tmp, size);

    lseek(f, b*size, 0);
    write(f, buf, size);
    
    lseek(f, a*size, 0);
    write(f, tmp, size);

    free(buf);
    free(tmp);    
}

int sys_partition(int f, int l, int r, int size){
    char* buf = (char*) calloc(size, sizeof(char));

    int pivot = l;
    char* pivot_value = (char*) calloc(size, sizeof(char));
    lseek(f, pivot*size, 0);
    read(f, pivot_value, size);
    sys_swap(f, pivot, r, size);

    int current_pos = l;
    int i;
    for(i = l; i < r; i++){
        lseek(f, i*size, 0);
        read(f, buf, size);
        if(strcmp(buf, pivot_value) < 0){
            sys_swap(f, i, current_pos, size);
            current_pos++;
        }
    }

    sys_swap(f, current_pos, r, size);

    free(buf);
    free(pivot_value);

    return current_pos;
}

void sys_quicksort(int f, int l, int r, int size){
    if(l < r){
        int i = sys_partition(f, l, r, size);
        sys_quicksort(f, l, i - 1, size);
        sys_quicksort(f, i + 1, r, size);
    }
}

void lib_swap(FILE* f, int a, int b, int size){
    char* buf = (char*) calloc(size, sizeof(char));
    char* tmp = (char*) calloc(size, sizeof(char));
    
    fseek(f, a*size, 0);
    fread(buf, sizeof(char), size, f);

    fseek(f, b*size, 0);
    fread(tmp, sizeof(char), size, f);

    fseek(f, b*size, 0);
    fwrite(buf, sizeof(char), size, f);
    
    fseek(f, a*size, 0);
    fwrite(tmp, sizeof(char), size, f);

    free(buf);
    free(tmp);    
}

int lib_partition(FILE* f, int l, int r, int size){
    char* buf = (char*) calloc(size, sizeof(char));

    int pivot = l;
    char* pivot_value = (char*) calloc(size, sizeof(char));
    fseek(f, pivot*size, 0);
    fread(pivot_value, sizeof(char), size, f);
    lib_swap(f, pivot, r, size);

    int current_pos = l;
    int i;
    for(i = l; i < r; i++){
        fseek(f, i*size, 0);
        fread(buf, sizeof(char), size, f);
        if(strcmp(buf, pivot_value) < 0){
            lib_swap(f, i, current_pos, size);
            current_pos++;
        }
    }

    lib_swap(f, current_pos, r, size);

    free(buf);
    free(pivot_value);

    return current_pos;
}

void lib_quicksort(FILE* f, int l, int r, int size){
    if(l < r){
        int i = lib_partition(f, l, r, size);
        lib_quicksort(f, l, i - 1, size);
        lib_quicksort(f, i + 1, r, size);
    }
}

double time_diff(clock_t start, clock_t end){
    return ((double) (end-start)/sysconf(_SC_CLK_TCK));
}

void perform_test(int number_of_records, int size, char* mode){
    FILE* results = fopen("wyniki.txt", "a");

    struct tms* start_time = (struct tms*) calloc(1, sizeof(struct tms));
    struct tms* sys_time = (struct tms*) calloc(1, sizeof(struct tms));
    struct tms* lib_time = (struct tms*) calloc(1, sizeof(struct tms));

    if(strcmp(mode, "sort") == 0){
        system("touch sys_sort");
        system("touch lib_sort");

        generate("sys_sort", number_of_records, size);
        FILE* sys_f_tmp = fopen("sys_sort", "r+");
        FILE* lib_f = fopen("lib_sort", "r+");
        lib_copy(sys_f_tmp, lib_f, number_of_records, size);

        fclose(sys_f_tmp);

        int sys_f = open("./sys_sort", O_RDWR);
        
        times(start_time);
        sys_quicksort(sys_f, 0, number_of_records-1, size);
        times(sys_time);
        lib_quicksort(lib_f, 0, number_of_records-1, size);
        times(lib_time);

        fclose(lib_f);
        close(sys_f);

        fprintf(results, "function: %s  records: %d   size: %d\n", mode, number_of_records, size);
        fprintf(results, "system function:\n");
        fprintf(results, "\tuser time: %f\n", time_diff(start_time -> tms_utime, sys_time -> tms_utime));
        fprintf(results, "\tsystem time: %f\n", time_diff(start_time -> tms_stime, sys_time -> tms_stime));
        fprintf(results, "lib function:\n");
        fprintf(results, "\tuser time:%f\n", time_diff(sys_time -> tms_utime, lib_time -> tms_utime));
        fprintf(results, "\tsystem time:%f\n\n", time_diff(sys_time -> tms_stime, lib_time -> tms_stime));
    
        system("rm sys_sort lib_sort");
    }
    else if(strcmp(mode, "copy") == 0){
        struct tms* start_time2 = (struct tms*) calloc(1, sizeof(struct tms));
        system("touch copy_source");
        system("touch copy_lib");
        system("touch copy_sys");

        generate("copy_source", number_of_records, size);
        int ss = open("./copy_source", O_RDONLY);
        int ds = open("./copy_sys", O_WRONLY);

        times(start_time);
        sys_copy(ss, ds, number_of_records, size);
        times(sys_time);

        close(ss);
        close(ds);
        
        
        FILE* sl = fopen("copy_source", "r");
        FILE* dl = fopen("copy_lib", "w");

        times(start_time2);
        lib_copy(sl, dl, number_of_records, size);
        times(lib_time);

        fclose(sl);
        fclose(dl);

        fprintf(results, "function: %s  records: %d   size: %d\n", mode, number_of_records, size);
        fprintf(results, "system function:\n");
        fprintf(results, "\tuser time: %f\n", time_diff(start_time -> tms_utime, sys_time -> tms_utime));
        fprintf(results, "\tsystem time: %f\n", time_diff(start_time -> tms_stime, sys_time -> tms_stime));
        fprintf(results, "lib function:\n");
        fprintf(results, "\tuser time:%f\n", time_diff(start_time2 -> tms_utime, lib_time -> tms_utime));
        fprintf(results, "\tsystem time:%f\n\n", time_diff(start_time2 -> tms_stime, lib_time -> tms_stime));
    
        system("rm copy_source copy_lib copy_sys");

        free(start_time2);
    }

    free(start_time);
    free(sys_time);
    free(lib_time);

    fclose(results);
}

int main(int argc, char* argv[]){
    
    int n = 1;
    while(n < argc){
        if(strcmp("generate", argv[n]) == 0){
            generate(argv[n+1], atoi(argv[n+2]), atoi(argv[n+3]));
            n += 4;
        }
        else if(strcmp("sort", argv[n]) == 0){
            if(strcmp(argv[n+4], "sys") == 0){
                char* file_path = (char*) calloc(strlen(argv[n+1]) + 2, sizeof(char));
                sprintf(file_path, "./%s", argv[n+1]);
                int f = open(file_path, O_RDWR);
                
                sys_quicksort(f, 0, atoi(argv[n+2]) - 1, atoi(argv[n+3]));

                close(f);
                free(file_path);
            }
            else if(strcmp(argv[n+4], "lib") == 0){
                FILE* f = fopen(argv[n+1], "r+");

                lib_quicksort(f, 0, atoi(argv[n+2]) - 1, atoi(argv[n+3]));

                fclose(f);
            }
            n += 5;
        }
        else if(strcmp("copy", argv[n]) == 0){
            if(strcmp(argv[n+5], "sys") == 0){
                char* file_path_source = (char*) calloc(strlen(argv[n+1]) + 2, sizeof(char));
                sprintf(file_path_source, "./%s", argv[n+1]);
                int sf = open(file_path_source, O_RDONLY);

                char* file_path_dest = (char*) calloc(strlen(argv[n+2]) + 2, sizeof(char));
                sprintf(file_path_dest, "./%s", argv[n+2]);
                int df = open(file_path_dest, O_WRONLY);

                sys_copy(sf, df, atoi(argv[n+3]), atoi(argv[n+4]));

                close(sf);
                close(df);
                free(file_path_source);
                free(file_path_dest);
            }
            else if(strcmp(argv[n+5], "lib") == 0){
                FILE* sf = fopen(argv[n+1], "r");
                FILE* df = fopen(argv[n+2], "w");

                lib_copy(sf, df, atoi(argv[n+3]), atoi(argv[n+4]));

                fclose(sf);
                fclose(df);
            }
            n += 6;
        }
        else{
            printf("unknown argument: %s, index: %d\n", argv[n], n);
            n++;
        }

    }
    if(n != 1){
        return 0;
    }
    /// tests 
    system("rm wyniki.txt");
    system("touch wyniki.txt");
    // small number of records, sorting
    perform_test(50000, 1, "sort");
    perform_test(50000, 4, "sort");
    perform_test(50000, 512, "sort");
    perform_test(50000, 1024, "sort");
    perform_test(50000, 4096, "sort");
    perform_test(50000, 8192, "sort");
    
    // big number of records, sorting
    perform_test(70000, 1, "sort");
    perform_test(70000, 4, "sort");
    perform_test(70000, 512, "sort");
    perform_test(70000, 1024, "sort");
    perform_test(70000, 4096, "sort");
    perform_test(70000, 8192, "sort");
    
    // small number of records, copying
    perform_test(10000, 1, "copy");
    perform_test(10000, 4, "copy");
    perform_test(10000, 512, "copy");
    perform_test(10000, 1024, "copy");
    perform_test(10000, 4096, "copy");
    perform_test(10000, 8192, "copy");

    // big num of records, copying;
    perform_test(250000, 1, "copy");
    perform_test(250000, 4, "copy");
    perform_test(250000, 512, "copy");
    perform_test(250000, 1024, "copy");
    perform_test(250000, 4096, "copy");
    perform_test(250000, 8192, "copy");
    
    return 0;
}