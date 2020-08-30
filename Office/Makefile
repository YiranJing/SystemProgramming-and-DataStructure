CC=clang
CFLAGS=-Werror=vla -Wextra -Wall -Wshadow -Wswitch-default -std=c11 -g
CFLAG_SAN=$(CFLAGS) -fsanitize=address
DEPS=queue.o stack.o
OBJ=office.o

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS_SAN)


office.o: $(DEPS)

clean:
	rm *.o
