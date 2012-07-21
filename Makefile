.PHONY: all clean

all: client server

clean:
	rm -f client server

client: client.c common.h
	gcc -Werror -Wall -g client.c -o client

server: server.c common.h
	gcc -Werror -Wall -g server.c -o server
