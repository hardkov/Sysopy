#define BUF_SIZE 2000
#define ELEM_SIZE 15
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/stat.h>

struct matrix{
    int rows;
    int cols;
    int** vals;
};

struct matrix* load_matrix(char* file){
    int col_counter, row_counter, start_comma_index;
    int i, r, c, end_comma_index;
    char* buf = (char*) calloc(BUF_SIZE,  sizeof(char));
    char* matrix_element = (char*) calloc(ELEM_SIZE, sizeof(char));
    FILE* f;
    struct matrix* m = (struct matrix*) calloc(1, sizeof(struct matrix));

    f = fopen(file, "r");
    col_counter = 0;
    row_counter = 0;
    start_comma_index = -1;
    end_comma_index = -1;

    fread(buf, sizeof(char), BUF_SIZE, f);
        
    i = 0;
    while(buf[i] != '\n'){
        if(buf[i] == ',') col_counter++;
        i++;
    }

    i = 0;
    while(i != BUF_SIZE && buf[i] != '\0'){
        if(buf[i] == '\n') row_counter++;
        i++;
    }
    
    m -> rows = row_counter;
    m -> cols = col_counter+1;

    m -> vals = (int**) calloc(row_counter, sizeof(int*));
    for(i = 0; i < row_counter; i++){
        m -> vals[i] = (int*) calloc(col_counter+1, sizeof(int)); 
    }

    i = 0;
    r = 0; // row index
    c = 0; // col index
    while(i != BUF_SIZE && buf[i] != '\0'){
        if(buf[i] == ',' || buf[i] == '\n'){
            start_comma_index = end_comma_index;
            end_comma_index = i;
            memcpy(matrix_element, buf+start_comma_index+1, end_comma_index - start_comma_index - 1);
            m -> vals[r][c] = atoi(matrix_element);
            memset(matrix_element, 0, strlen(matrix_element));
            c++; 
        }
        if(buf[i] == '\n'){
            r++;
            c = 0;
        }
        i++; 
    }

    fclose(f);
    free(buf);
    free(matrix_element);

    return m;
}

int get_task(char* com_file, int* m_index){
    int f, i, cp_index, c_index, index_to_update;
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    char* matrix_element = (char*) calloc(ELEM_SIZE, sizeof(char));

    f = open(com_file, O_RDWR);
    cp_index = -1;
    c_index = -1;
    index_to_update = -1;
    *m_index = -1;

    flock(f, LOCK_EX);

    read(f, buf, BUF_SIZE);
    
    i = 0;
    while(i != BUF_SIZE && buf[i] != '\0'){
        if(buf[i] == '-'){
            index_to_update = i;
            i += 2;
            cp_index = i;
            while(buf[cp_index] != '_'){
                cp_index++;
            }
            memcpy(matrix_element, buf+i, cp_index-i);
            *m_index = atoi(matrix_element);
            memset(matrix_element, 0, strlen(matrix_element));
            cp_index++;
            i = cp_index;
            while(buf[cp_index] != '\n'){
                cp_index++;
            }
            memcpy(matrix_element, buf+i, cp_index - i);
            c_index = atoi(matrix_element);
            break;

        }
        i++;
    }

    if(index_to_update != -1){
        lseek(f, index_to_update, 0);
        write(f, "+", 1);
    }
    
    free(buf);
    free(matrix_element);
    flock(f, LOCK_UN);
    close(f);

    return c_index;
}

void prepare_files(struct matrix** input_matrices, int number_of_matrices){
    int i,j,k,cols,rows;
    FILE* f;
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    
    for(i = 0; i < number_of_matrices; i++){
        sprintf(buf, "out%d.txt", i);
        f = fopen(buf, "w");
        rows = input_matrices[2*i] -> rows;
        cols = input_matrices[2*i+1] -> cols;
        
        for(j = 0; j < rows; j++){
            for(k = 0; k < cols-1; k++){
                fwrite(",", sizeof(char), 1, f);
            }
            fwrite("\n", sizeof(char), 1, f);
        }
        fclose(f);
    }

    free(buf);
}

void split_multiply_column(struct matrix* m_in1, struct matrix* m_in2, int c_index, char* file){
    int i, j, sum;
    FILE* f = fopen(file, "w");

    for(i = 0; i < m_in1 -> rows; i++){
        sum = 0;
        for(j = 0; j < m_in1 -> cols; j++){
            sum += (m_in1 -> vals[i][j]) * (m_in2 -> vals[j][c_index]);
        }
        fprintf(f, "%d\n", sum);
    }
    fclose(f);
}

void print_into_file(int r_index, int c_index, int sum, char* file){
    int f, bytes_to_print;
    int new_line_counter, comma_counter, i;
    char* buf_old = (char*) calloc(BUF_SIZE/2, sizeof(char));
    char* buf_new = (char*) calloc(BUF_SIZE, sizeof(char));
    char* matrix_element = (char*) calloc(ELEM_SIZE, sizeof(char));

    new_line_counter = 0;   // this corresponds to rows (and to cols when it's the last column)
    comma_counter = 0;       // this corresponds to cols
    f = open(file, O_RDWR);
    
    flock(f, LOCK_EX);
    read(f, buf_old, BUF_SIZE/2);

    i = 0;
    while(i != BUF_SIZE/2 && buf_old[i] != '\0'){
        if(new_line_counter == r_index && comma_counter == c_index) break;
        else if(buf_old[i] == ',') comma_counter++;
        else if(buf_old[i] == '\n'){
            new_line_counter++;
            comma_counter = 0;
        }
        i++;
    }
    
    memcpy(buf_new, buf_old, i);
    sprintf(matrix_element, "%d", sum);
    strcat(buf_new, matrix_element);
    strcat(buf_new, buf_old+i);

    bytes_to_print = 0;
    while(buf_new[bytes_to_print] != '\0' && bytes_to_print != BUF_SIZE) bytes_to_print++;

    lseek(f, 0, 0);
    write(f, buf_new, bytes_to_print);

    free(buf_old);
    free(buf_new);
    free(matrix_element);
    flock(f, LOCK_UN);
    close(f);
}

void common_multiply_column(struct matrix* m_in1, struct matrix* m_in2, int c_index, char* file){
    int i, j, sum;

    for(i = 0; i < m_in1 -> rows; i++){
        sum = 0;
        for(j = 0; j < m_in1 -> cols; j++){
            sum += (m_in1 -> vals[i][j]) * (m_in2 -> vals[j][c_index]);
        }
        print_into_file(i, c_index, sum, file);
    }
}

int do_the_math(struct matrix** input_matrices, char* com_file, char* mode){
    int multiplication_counter, c_index, m_index, raport;
    char* buf;
    struct rusage* usage = (struct rusage*) calloc(1, sizeof(struct rusage));

    multiplication_counter = 0;
    c_index = -1;
    m_index = -1;
    buf = (char*) calloc(BUF_SIZE, sizeof(char));

    while(1){ // here i set the limits, ugly while
        c_index = get_task(com_file, &m_index);
        if(c_index == -1) break;
        if(strcmp("split", mode) == 0){
            sprintf(buf, "touch split_%d_%d", m_index, c_index);  // touch command
            system(buf);
            split_multiply_column(input_matrices[2*m_index], input_matrices[2*m_index + 1], c_index, buf+6);    
        }
        else if(strcmp("common", mode) == 0){
            sprintf(buf, "out%d.txt", m_index);  // file name
            common_multiply_column(input_matrices[2*m_index], input_matrices[2*m_index + 1], c_index, buf);
        }
        multiplication_counter++;
    }
    
    raport = open("raport.txt", O_WRONLY);
    flock(raport, LOCK_EX);
    
    getrusage(RUSAGE_SELF, usage);
    sprintf(buf, "PID: %d\nSYS CPU TIME USAGE (us): %ld\nUSER CPU TIME USAGE (us): %ld \n\n",
    getpid(), usage -> ru_stime.tv_usec, usage -> ru_utime.tv_usec);
    lseek(raport, 0, 2);
    write(raport, buf, strlen(buf));

    flock(raport, LOCK_UN);
    close(raport);
    free(buf);

    return multiplication_counter;
}

struct matrix** create_com_file(char* list, int* number_of_matricies){
    int matrix_counter, i, j;
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));    
    FILE* list_file;
    FILE* com_file;
    struct matrix** input_matrices;

    matrix_counter = 0;
    list_file = fopen(list, "r");
    system("touch com_file.txt");
    com_file = fopen("com_file.txt", "w");
    
    fread(buf, sizeof(char), BUF_SIZE, list_file);

    i = 0;
    while(i != BUF_SIZE && buf[i] != '\0'){
        if(buf[i] == '\n') matrix_counter++;
        i++;
    }

    input_matrices = (struct matrix**) calloc(2*(matrix_counter), sizeof(struct matrix*));

    for(i = 0; i < matrix_counter; i++){
        sprintf(buf, "in%d.txt", 2*i);
        input_matrices[2*i] = load_matrix(buf);
        sprintf(buf, "in%d.txt", 2*i+1);
        input_matrices[2*i+1] = load_matrix(buf);
        for(j = 0; j < input_matrices[2*i+1] -> cols; j++){
            fprintf(com_file, "-_%d_%d\n", i, j);
        }
    }

    *number_of_matricies = matrix_counter;

    free(buf);
    fclose(list_file);
    fclose(com_file);
    
    return input_matrices;
}

int main(int argc, char* argv[]){
    if(argv != 6){
        printf("usage: ./macierz list.txt number_of_workers mode cpu_limit mem_limit(MB)");
        return 0;
    }
    
    char* list = (char*) calloc(strlen(argv[1]), sizeof(char));
    char* mode = (char*) calloc(strlen(argv[4]), sizeof(char));
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    int number_of_workers, status, i, number_of_matricies;
    pid_t* workers;
    pid_t pid_tmp;
    struct matrix** input_matrices;
    struct rlimit* rl = (struct rlimit*) calloc(1, sizeof(struct rlimit));

    strcpy(list, argv[1]);
    number_of_workers = atoi(argv[2]);
    strcpy(mode, argv[3]);
    
    pid_tmp = 1;
    input_matrices = create_com_file(list, &number_of_matricies);
    workers = (pid_t*) calloc(number_of_workers, sizeof(pid_t));

    if(strcmp("common", mode) == 0) prepare_files(input_matrices, number_of_matricies);
    
    system("rm raport.txt");
    system("touch raport.txt");

    for(i = 0; i < number_of_workers; i++){
        pid_tmp = fork();
        if(pid_tmp == 0){
            rl -> rlim_max = atoi(argv[4]);
            rl -> rlim_cur = rl -> rlim_max - 1;
            if(setrlimit(RLIMIT_CPU, rl) != 0) printf("setrlimit error\n");
    
            rl -> rlim_max = atoi(argv[5]) * 1000000;
            rl -> rlim_cur = rl -> rlim_max - 1;
            if(setrlimit(RLIMIT_AS, rl) != 0) printf("setrlimit error\n");

            return do_the_math(input_matrices, "com_file.txt", mode);   // it is executed by a child
        }
        else{
            workers[i] = pid_tmp;
        }
    }

    for(i = 0; i < number_of_workers; i++){
        status = -1;
        waitpid(workers[i], &status, 0);
        printf("Proces %d wykonal %d mnozen macierzy\n", workers[i], WEXITSTATUS(status));
    }

    if(strcmp("split", mode) == 0){
        for(i = 0; i < number_of_matricies; i++){
            sprintf(buf, "paste split_%d* -d',' > out%d.txt", i, i);
            system(buf);
        }
    }

    free(rl);
    free(workers);
    free(buf);
    free(mode);
    free(list);
    
    return 0;
}