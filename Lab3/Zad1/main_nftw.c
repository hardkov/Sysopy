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
#include <sys/types.h>
#include <sys/wait.h>

static int fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    char* buf = (char*) calloc(500, sizeof(char));

    if(typeflag == FTW_D && fork() == 0){
        printf("%d, %s\n", getpid(), fpath);
        sprintf(buf, "ls -l %s", fpath);
        system(buf);
        return 0;
    }

    wait(NULL);

    free(buf);

    return 0;
}

int main(int argc, char* argv[]){
    if(argc == 2) nftw(argv[1], fn, 20, FTW_PHYS);
    else nftw(".", fn, 20, FTW_PHYS);
    return 0;
}