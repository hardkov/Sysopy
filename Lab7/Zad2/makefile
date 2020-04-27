CC = gcc -Wall

w1:
	$(CC) worker1.c -o worker1 -lrt -pthread

w2:
	$(CC) worker2.c -o worker2 -lrt -pthread

w3:
	$(CC) worker3.c -o worker3 -lrt -pthread

m:
	$(CC) main.c -o main -lrt -pthread

all:
	$(CC) worker1.c -o worker1 -lrt -pthread 
	$(CC) worker2.c -o worker2 -lrt -pthread 
	$(CC) worker3.c -o worker3 -lrt -pthread