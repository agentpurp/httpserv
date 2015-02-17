all: httpserv

httpserv: httpserv.c
	gcc -Wall -o httpserv httpserv.c

clean:
	rm httpserv
