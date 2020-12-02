# run makefile
# $ make

CC=gcc
CFLAGS=-Wall -Werror -std=c11 -g -pthread
TARGET=program

all: $(TARGET)
	
program: Lab_exercise.c
		$(CC) $(CFLAGS) $^ -o $@
		./$@ all

clean:
		rm -f *.o
		rm -f program


