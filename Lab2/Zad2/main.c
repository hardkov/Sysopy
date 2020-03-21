#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>


long int get_links_num(struct stat* file_info){
    return file_info -> st_nlink; 
}

char* get_file_type(struct stat* file_info){
    int mode = file_info -> st_mode;

    if(S_ISREG(mode)) return "file";
    else if(S_ISLNK(mode)) return "slink";  
    else if(S_ISSOCK(mode)) return "sock"; 
    else if(S_ISDIR(mode)) return "dir";
    else if(S_ISCHR(mode)) return "char dev";
    else if(S_ISBLK(mode)) return "block dev";
    else if(S_ISFIFO(mode)) return "fifo";
    else return NULL;
}

long int get_file_size(struct stat* file_info){
    return file_info -> st_size;
}

char* get_acc_date(struct stat* file_info){
    int buf_size = 80;
    char* buf = (char*) calloc(buf_size, sizeof(char));
    struct tm ts;
    
    ts = *localtime(&(file_info -> st_atime));
    strftime(buf, buf_size, "%Y-%m-%d", &ts);

    return buf;
}

char* get_mod_date(struct stat* file_info){
    int buf_size = 80;
    char* buf = (char*) calloc(buf_size, sizeof(char));
    struct tm ts;
    
    ts = *localtime(&(file_info -> st_mtime));
    strftime(buf, buf_size, "%Y-%m-%d", &ts);

    return buf;   
}

char* parse_file_info(struct stat* file_info){ 
     
    char* buf = (char*) calloc(500, sizeof(char));     

    sprintf(buf,  "%ld,  %s,  %ld,  a: %s,  m: %s", get_links_num(file_info),
        get_file_type(file_info), get_file_size(file_info), get_acc_date(file_info), get_mod_date(file_info));

    return buf;
}

int is_file_valid(struct stat* file_info, int atime_min, int atime_exact, int atime_max, int mtime_min, int mtime_exact, int mtime_max){
    int mtime = file_info -> st_mtime;
    int atime = file_info -> st_atime;
    int return_value = 1; // true at start
    time_t current_time;
    int time_diff_a;
    int time_diff_m;

    current_time = time(NULL);
    time_diff_a = (int) difftime(current_time, atime)/86400;
    time_diff_m = (int) difftime(current_time, mtime)/86400;

    if((atime_min >= time_diff_a) && (atime_min != -1)) return_value = 0;
    if((atime_exact != time_diff_a) && (atime_exact != -1)) return_value = 0;
    if((atime_max <= time_diff_a) && (atime_max != -1)) return_value = 0;
    if((mtime_min >= time_diff_m) && (mtime_min != -1)) return_value = 0;
    if((mtime_exact != time_diff_m) && (mtime_exact != -1)) return_value = 0;
    if((mtime_max <= time_diff_m) && (mtime_max != -1)) return_value = 0;
    
    return return_value;
}


// -1 means the parameter should not be considered
void dir_find_recursive(char* base_path, int atime_min, int atime_exact, int atime_max, int mtime_min, int mtime_exact, int mtime_max, int maxdepth){
    if(maxdepth == 0) return;
    char* path = (char*) calloc(500, sizeof(char));
    struct dirent* fni = NULL;
    DIR* dir = opendir(base_path);
    struct stat* file_info = (struct stat*) calloc(1, sizeof(struct stat));

    if(dir == NULL) return;
    while((fni = readdir(dir)) != NULL){
        if(strcmp(fni -> d_name, ".") != 0 && strcmp(fni -> d_name, "..") != 0){
            strcpy(path, base_path);
            strcat(path, "/");
            strcat(path, fni -> d_name);

            lstat(path, file_info);
            if(is_file_valid(file_info, atime_min, atime_exact, atime_max, mtime_min, mtime_exact, mtime_max) == 1){
                printf("%s,  %s\n", path, parse_file_info(file_info));
            }
            dir_find_recursive(path, atime_min, atime_exact, atime_max, mtime_min, mtime_exact, mtime_max, maxdepth-1);
        }
    }

    free(file_info);
    closedir(dir);
}

void dir_find(char* base_path, int atime_min, int atime_exact, int atime_max, int mtime_min, int mtime_exact, int mtime_max, int maxdepth){
    struct stat* file_info = (struct stat*) calloc(1, sizeof(struct stat));

    lstat(base_path, file_info);

    printf("%s,  %s\n", base_path, parse_file_info(file_info));
    
    free(file_info);
    dir_find_recursive(base_path, atime_min, atime_exact, atime_max, mtime_min, mtime_exact, mtime_max, maxdepth);
}



int main(int argc, char* argv[]){
    int maxdepth = -1;
    int atime_min = -1;
    int atime_exact = -1;
    int atime_max = -1;
    int mtime_min = -1;
    int mtime_exact = -1;
    int mtime_max = -1;
    char* base_path = (char*) calloc(100, sizeof(char));
    strcpy(base_path, ".");

    int n = 1;
    while(n < argc){
        if(strcmp(argv[n], "-maxdepth") == 0){
            maxdepth = atoi(argv[n+1]);
            n += 2;
        }
        else if(strcmp(argv[n], "-atime") == 0){
            if(argv[n+1][0] == '+'){
                atime_min = atoi(argv[n+1]);
            }
            else if(argv[n+1][0] == '-'){
                atime_max = (-1)*atoi(argv[n+1]);
            }
            else{
                atime_exact = atoi(argv[n+1]);
            }
            n += 2;
        }
        else if(strcmp(argv[n], "-mtime") == 0){
            if(argv[n+1][0] == '+'){
                mtime_min = atoi(argv[n+1]);
            }   
            else if(argv[n+1][0] == '-'){
                mtime_max = (-1)*atoi(argv[n+1]);
            }
            else{
                mtime_exact = atoi(argv[n+1]);
            }
            n += 2;
        }
        else if(n==1){
            strcpy(base_path, argv[n]);
            n++;
        }
        else{
            n++;
        }
    }
    dir_find(base_path, atime_min, atime_exact, atime_max, mtime_min, mtime_exact, mtime_max, maxdepth);
    free(base_path);

    return 0;
}