CC=clang
CFLAGS=-Werror=vla -Wextra -Wall -Wshadow -Wswitch-default -std=c11 -g
CFLAG_SAN=$(CFLAGS) -fsanitize=address
DEPS=
OBJ=channel.o

channel.o: channel.c channel.h $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm *.o
