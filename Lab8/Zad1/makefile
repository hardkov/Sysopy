pgma:
	gcc -c -Wall pgma_io.c

m:
	gcc -c -Wall main.c
	gcc main.o pgma_io.o -o main -lm -lpthread

clean:
	rm hist*

test:
	./main 1 block t.pgm hist0
	./main 1 sign t.pgm hist1
	./main 1 interleaved t.pgm hist2
	./main 2 block t.pgm hist0
	./main 2 sign t.pgm hist1
	./main 2 interleaved t.pgm hist2
	./main 4 block t.pgm hist0
	./main 4 sign t.pgm hist1
	./main 4 interleaved t.pgm hist2
	./main 8 block t.pgm hist0
	./main 8 sign t.pgm hist1
	./main 8 interleaved t.pgm hist2