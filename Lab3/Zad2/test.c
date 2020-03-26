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

struct matrix{
    int** vals;
    int cols;
    int rows;
};


void prepare_files(struct matrix** input_matrices, int number_of_matrices){
    int i,j,k,cols,rows;
    FILE* f;
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    
    for(i = 0; i < number_of_matrices; i++){
        sprintf(buf, "in%d.txt", 2*i);
        f = fopen(buf, "wa");
        rows = input_matrices[2*i] -> rows;
        cols = input_matrices[2*i] -> cols;
        
        for(j = 0; j < rows; j++){
            for(k = 0; k < cols-1; k++){
                fwrite(",", sizeof(char), 1, f);
            }
            fwrite("\n", sizeof(char), 1, f);
        }
        fclose(f);

        sprintf(buf, "in%d.txt", 2*i+1);
        f = fopen(buf, "wa");
        rows = input_matrices[2*i+1] -> rows;
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


void create_random_matricies(int min_size, int max_size, struct matrix** input_matricies, int number_of_matricies){
    int cols1, cols2, rows1, rows2, i, j, k;

    for(k = 0; k < number_of_matricies; k++){
        struct matrix * m1 = (struct matrix*) calloc(1, sizeof(struct matrix));
        struct matrix * m2 = (struct matrix*) calloc(1, sizeof(struct matrix));
        
        cols1 = rand() % (max_size - min_size + 1) + min_size;
        rows1 = rand() % (max_size - min_size + 1) + min_size;         
        cols2 = rand() % (max_size - min_size + 1) + min_size;
        rows2 = cols1;

        m1 -> cols = cols1;
        m1 -> rows = rows1;
        m1 -> vals = (int**) calloc(m1 -> rows, sizeof(int*));

        for(i = 0; i < rows1; i++){
            m1 -> vals[i] = (int*) calloc(cols1, sizeof(int));
            for(j = 0; j < cols1; j++) m1 -> vals[i][j] = rand()%100;
        }

        m2 -> cols = cols2;
        m2 -> rows = rows2;
        m2 -> vals = (int**) calloc(m2 -> rows, sizeof(int*));

        for(i = 0; i < rows2; i++){
            m2 -> vals[i] = (int*) calloc(cols2, sizeof(int));
            for(j = 0; j < cols2; j++) m2 -> vals[i][j] = rand()%100;
        }

        input_matricies[2*k] = m1;
        input_matricies[2*k+1] = m2;
    }
}

void create_matricies_files(struct matrix** input_matricies, int number_of_matricies){
    int i, j, k;
    struct matrix* m1;
    struct matrix* m2;
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));

    prepare_files(input_matricies, number_of_matricies);

    for(i = 0; i < number_of_matricies; i++){
        sprintf(buf, "touch out%d.txt", i);
        system(buf);    

        m1 = input_matricies[2*i];
        sprintf(buf, "in%d.txt", 2*i);
        
        for(j = 0; j < m1 -> rows; j++){
            for(k = 0; k < m1 -> cols; k++){
                print_into_file(j, k, m1 -> vals[j][k], buf);
            }
        }

        m2 = input_matricies[2*i+1];
        sprintf(buf, "in%d.txt", 2*i+1);
        
        for(j = 0; j < m2 -> rows; j++){
            for(k = 0; k < m2 -> cols; k++){
                print_into_file(j, k, m2 -> vals[j][k], buf);
            }
        }
    }
    free(buf);
}

int matrix_cmp(struct matrix* m1, struct matrix* m2){
    int i,j;
    if(m1 -> cols != m2 -> cols || m1 -> rows != m2 -> rows) return -1;

    for(i = 0; i < m1 -> rows; i++){
        for(j = 0; j < m2 -> cols; j++){
            if(m1 -> vals[i][j] != m2 -> vals[i][j]) return -1;
        }
    }

    return 0;
}

struct matrix* multiply(struct matrix* m1, struct matrix* m2){
    int sum, i, j, k;
    struct matrix* m3 = (struct matrix*) calloc(1, sizeof(struct matrix));
    

    m3 -> rows = m1 -> rows;
    m3 -> cols = m2 -> cols;
    m3 -> vals = (int**) calloc(m3 -> rows, sizeof(int*));

    for(i = 0; i < m1 -> rows; i++){
        m3 -> vals[i] = (int*) calloc(m3 -> cols, sizeof(int));
        for(j = 0; j < m2 -> cols; j++){
            sum = 0;
            for(k = 0; k < m1 -> cols; k++){
                
                
                sum += (m1 -> vals[i][k]) * (m2 -> vals[k][j]);
            }
            m3 -> vals[i][j] = sum;
        }
    }

    return m3;
}

void perform_test(struct matrix** input_matricies, int number_of_matricies, int number_of_workers, int time_out, char* mode){
    int i;
    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    system("make m");
    sprintf(buf, "./macierz list.txt %d %d %s", number_of_workers, time_out, mode);  
    system(buf);
    struct matrix** output_matrix_actual = (struct matrix**) calloc(number_of_matricies, sizeof(struct matrix*));
    struct matrix** output_matrix_expected = (struct matrix**) calloc(number_of_matricies, sizeof(struct matrix*));
    for(i = 0; i < number_of_matricies; i++){
        sprintf(buf, "out%d.txt", i);
        output_matrix_actual[i] = load_matrix(buf);
        output_matrix_expected[i] = multiply(input_matricies[2*i], input_matricies[2*i+1]);
        if(matrix_cmp(output_matrix_expected[i], output_matrix_actual[i]) != 0){
            printf("in%d.txt razy in%d.txt to nie out%d.txt\n", 2*i, 2*i+1, i);
        }
    }
}

void create_list_file(int number_of_matricies){
    int i;
    FILE* f;

    system("touch list.txt");
    f = fopen("list.txt", "w");
    
    for(i = 0; i < number_of_matricies; i++){
        fprintf(f, "in%d.txt,in%d.txt,out%d.txt\n", 2*i, 2*i+1, i);
    }

    fclose(f);
}

int main(int argc, char* argv[]){

    srand(time(NULL));

    int number_of_matricies, min_size, max_size;
    struct matrix** input_matricies;

    min_size = atoi(argv[1]);
    max_size = atoi(argv[2]);
    number_of_matricies = atoi(argv[3]);
    input_matricies = (struct matrix**) calloc(2*number_of_matricies, sizeof(struct matrix*));

    create_random_matricies(min_size, max_size, input_matricies, number_of_matricies);
    create_matricies_files(input_matricies, number_of_matricies);
    create_list_file(number_of_matricies);
    perform_test(input_matricies, number_of_matricies, 5, 100, "split");

    return 0;
}