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