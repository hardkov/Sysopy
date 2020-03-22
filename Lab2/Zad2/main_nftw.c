#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>


long int get_links_num(const struct stat* file_info){
    return file_info -> st_nlink; 
}

char* get_file_type(const struct stat* file_info){
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

long int get_file_size(const struct stat* file_info){
    return file_info -> st_size;
}

char* get_acc_date(const struct stat* file_info){
    int buf_size = 80;
    char* buf = (char*) calloc(buf_size, sizeof(char));
    struct tm ts;
    
    ts = *localtime(&(file_info -> st_atime));
    strftime(buf, buf_size, "%Y-%m-%d", &ts);

    return buf;
}

char* get_mod_date(const struct stat* file_info){
    int buf_size = 80;
    char* buf = (char*) calloc(buf_size, sizeof(char));
    struct tm ts;
    
    ts = *localtime(&(file_info -> st_mtime));
    strftime(buf, buf_size, "%Y-%m-%d", &ts);

    return buf;   
}

char* parse_file_info(const struct stat* file_info){ 
     
    char* buf = (char*) calloc(500, sizeof(char));     

    sprintf(buf,  "%ld,  %s,  %ld,  a: %s,  m: %s", get_links_num(file_info),
        get_file_type(file_info), get_file_size(file_info), get_acc_date(file_info), get_mod_date(file_info));

    return buf;
}

int is_file_valid(const struct stat* file_info, int atime_min, int atime_exact, int atime_max, int mtime_min, int mtime_exact, int mtime_max){
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

int maxdepth;
int atime_min;
int atime_exact; 
int atime_max; 
int mtime_min;
int mtime_exact; 
int mtime_max;

static int fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    if(is_file_valid(sb, atime_min, atime_exact, atime_max, mtime_min, mtime_exact, mtime_max) && ftwbuf -> level <= maxdepth){
        printf("%s,  %s\n", fpath, parse_file_info(sb));
    }
    
    return 0;
}

int main(int argc, char* argv[]){
    maxdepth = 20;
    atime_min = -1;
    atime_exact = -1;
    atime_max = -1;
    mtime_min = -1;
    mtime_exact = -1;
    mtime_max = -1;
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
    nftw(base_path, fn, 20, FTW_PHYS);
    
    free(base_path);

    return 0;
}