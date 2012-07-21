.PHONY: all clean

all: client server

clean:
	rm -f client server

client: client.c
	gcc -Werror -Wall -g client.c -o client

server: server.c
	gcc -Werror -Wall -g server.c -o server
