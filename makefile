all: server client

server: server.c mysocket.o
	gcc server.c -o server mysocket.o

client: client.c mysocket.o
	gcc client.c -o client mysocket.o

mysocket: mysocket.c
	gcc -c mysocket.c

clean:
	rm mysocket.o