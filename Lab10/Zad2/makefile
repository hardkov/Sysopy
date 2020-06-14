CC=gcc -Wall

all: s c t

c:
	$(CC) -o client client.c utils.c -lpthread

s:
	$(CC) -o server server.c utils.c -lpthread

t: client server
	./server 9898 sock

clean:
	rm -f client server sock
