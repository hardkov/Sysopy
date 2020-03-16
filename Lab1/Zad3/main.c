#include "mylib.h"
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

double time_diff(clock_t start, clock_t end){
    return ((double) (end-start)/sysconf(_SC_CLK_TCK));
}

/*
void create_report(clock_t* times, char* file1, int size){
    char* report_file_name = (char*) calloc(20, sizeof(char));
    char* touch_command = (char*) calloc(strlen(report_file_name) + 8);
    char* file_input = NULL;

    strcpy(report_file_name, "report_");

    if(strcmp(file1, "diff1.txt") == 0) strcat(report_file_name, "diff_");
    else strcat(report_file_name, "similar_");

    if(size == 2) strcat(report_file_name, "s");
    else if(size == 10) strcat(report_file_name, "m");
    else(size == 50) strcat(report_file_name, "l");

    strcpy(touch_command, "touch ");
    strcat(touch_command, report_file_name);

    system(touch_command);
    FILE* f = fopen(report_file_name, 'w');

    fprintf(f, "comparing time: %f \n", time_diff(times[0], times[1]));
    fprintf(f, "saving time: %f \n", time_diff(times[0], times[2]));
    fprintf(f, "deleting time: %f \n", time_diff(times[0], times[3]));
    fprintf(f, "s and d time: %f", time_diff(times[0], times[4]));
}
*/

void perform_test(int size, char* file1, char* file2){
    struct block_container* bl_c = create_table(size/2);
    char** files = (char**) calloc(size, sizeof(char*));

    struct tms* buf =(struct tms*) calloc(1, sizeof(struct tms));
    // for ex. start_time[0] - user time, start_time[1] - system time, start_time[2] - real time,  and so on
    clock_t* start_time = calloc(3, sizeof(clock_t));
    clock_t* comparing_time = calloc(3, sizeof(clock_t));
    clock_t* saving_time = calloc(3, sizeof(clock_t));
    clock_t* deleting_time = calloc(3, sizeof(clock_t));
    clock_t* s_and_d_time = calloc(3, sizeof(clock_t)); // stands for saving and deleting

    int i;
    for(i = 0; i < size; i++){
        if(i%2 == 0) files[i] = file1;
        else files[i] = file2;
    }

    start_time[2] = times(buf); 
    start_time[0] = buf -> tms_utime;
    start_time[1] = buf -> tms_stime;

    compare_files(files, size);

    comparing_time[2] = times(buf);
    comparing_time[0] = buf -> tms_utime;
    comparing_time[1] = buf -> tms_stime;

    for(i = 0; i < size/2; i++){
        char* tmp_file_name = (char*) calloc(10, sizeof(char));
        sprintf(tmp_file_name, "tmp_file%d", i);
        create_block(tmp_file_name, bl_c);
    }

    saving_time[2] = times(buf);
    saving_time[0] = buf -> tms_utime;
    saving_time[1] = buf -> tms_stime;

    for(i = 0; i < size/2; i++){
        remove_block(bl_c, i);
    }

    deleting_time[2] = times(buf); 
    deleting_time[0] = buf -> tms_utime;
    deleting_time[1] = buf -> tms_stime;

    create_block("tmp_file0", bl_c);
    remove_block(bl_c, 0);
    create_block("tmp_file0", bl_c);
    remove_block(bl_c, 0);
    create_block("tmp_file0", bl_c);
    remove_block(bl_c, 0);
    create_block("tmp_file0", bl_c);
    remove_block(bl_c, 0);
    
    s_and_d_time[2] = times(buf);
    s_and_d_time[0] = buf -> tms_utime;
    s_and_d_time[1] = buf -> tms_stime;
    
    system("make clean");

    FILE* f = fopen("raport2.txt", "a");

    fprintf(f, "size: %d   file difference: ", size);
    if(strcmp(file1, "diff1.txt") == 0) fprintf(f, "different\n\n");
    else if(strcmp(file1, "semidiff1.txt") == 0) fprintf(f, "semi-different\n\n");
    else fprintf(f, "similar\n\n");

    fprintf(f, "user times:\n"); 
    fprintf(f, "comparing time: %f \n", time_diff(start_time[0], comparing_time[0]));
    fprintf(f, "saving time: %f \n", time_diff(comparing_time[0], saving_time[0]));
    fprintf(f, "deleting time: %f \n", time_diff(saving_time[0], deleting_time[0]));
    fprintf(f, "s and d time: %f\n\n", time_diff(deleting_time[0], s_and_d_time[0]));

    fprintf(f, "system times:\n"); 
    fprintf(f, "comparing time: %f \n", time_diff(start_time[1], comparing_time[1]));
    fprintf(f, "saving time: %f \n", time_diff(comparing_time[1], saving_time[1]));
    fprintf(f, "deleting time: %f \n", time_diff(saving_time[1], deleting_time[1]));
    fprintf(f, "s and d time: %f\n\n", time_diff(deleting_time[1], s_and_d_time[1]));

    fprintf(f, "real times:\n"); 
    fprintf(f, "comparing time: %f \n", time_diff(start_time[2], comparing_time[2]));
    fprintf(f, "saving time: %f \n", time_diff(comparing_time[2], saving_time[2]));
    fprintf(f, "deleting time: %f \n", time_diff(saving_time[2], deleting_time[2]));
    fprintf(f, "s and d time: %f\n\n", time_diff(deleting_time[2], s_and_d_time[2]));

    fclose(f);
}

int main(int argc, char* argv[]){
    int n = 1;
    struct block_container* bl_c = NULL;
    while(n < argc){
        if(strcmp(argv[n], "create_table") == 0){
            bl_c = create_table(atoi(argv[n+1]));
            n += 2;
        }
        else if(strcmp(argv[n], "compare_files") == 0){
            int size = atoi(argv[n+1]);
            char** files = (char**) calloc(size, sizeof(char*));
            
            memcpy(files, argv+n+2, size*sizeof(char*));                    //look here for bugs
            compare_files(files, size);
            
            int i;
            for(i = 0; i < size; i++){
                char* tmp_file_name = (char*) calloc(10, sizeof(char));
                sprintf(tmp_file_name, "tmp_file%d", i);
                create_block(tmp_file_name, bl_c);
            }
            n += size + 2;
        }
        else if(strcmp(argv[n], "remove_block") == 0){
            remove_block(bl_c, atoi(argv[n+1]));
            n += 2;
        }
        else if(strcmp(argv[n], "remove_operation") == 0){
            remove_operation(atoi(argv[n+1]), atoi(argv[n+2]), bl_c);
            n += 3;
        }
        else{
            n++;
        }
    }

    //////
    if(n != 1){
        return 0;
    }
    //////
    ////// this part will only work if no arguments were specified (testing part)
    system("rm raport2.txt"); 
    system("touch raport2.txt");
    //------ similar files
    ////// small seq (10 files)
    
    perform_test(10, "similar1.txt", "similar2.txt");

    ////// medium seq(50 files)

    perform_test(50, "similar1.txt", "similar2.txt");

    ////// large seq(150 files)

    perform_test(150, "similar1.txt", "similar2.txt");

    //------ semidiff files
    ////// small seq(10 files)

    perform_test(10, "semidiff1.txt", "semidiff2.txt");

    ////// medium seq(50 files)

    perform_test(50, "semidiff1.txt", "semidiff2.txt");

    ////// large seq(150 files)

    perform_test(150, "semidiff1.txt", "semidiff2.txt");

    //------ different files
    ////// small seq(50 files)

    perform_test(10, "diff1.txt", "diff2.txt");

    ////// medium seq(50 files)

    perform_test(50, "diff1.txt", "diff2.txt");

    ////// large seq(150 files)

    perform_test(150, "diff1.txt", "diff2.txt"); 

    return 0;
}