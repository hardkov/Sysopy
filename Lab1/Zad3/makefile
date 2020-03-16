CC = gcc -Wall -std=c11

all: staticlib sharedlib clean

staticlib:
	$(CC) -c mylib.c
	ar rcs mylib.a mylib.o

sharedlib:
	$(CC) -fPIC -c mylib.c
	$(CC) -shared -o libmylib.so mylib.o

clean:
	rm -f *.o tmp_file*

test_static:
	make staticlib
	make clean
	$(CC) main.c mylib.a -o main
	./main

test_shared:
	make sharedlib
	make clean
	$(CC) -L./ -Wl,-rpath=./ -o main main.c -lmylib
	./main

test_dynamic:
	make sharedlib
	make clean
	$(CC) -o main_d main_dynamic.c -ldl -I .
	./main_d

test_opO:
	make sharedlib
	make clean
	$(CC) -Os -o main_d main_dynamic.c -ldl -I .
	./main_d

test_opO2:
	make staticlib
	make clean
	$(CC) -O2 main.c mylib.a -o main 
	./main

test_opOs:
	make staticlib
	make clean
	$(CC) -Os main.c mylib.a -o main	
	./main
