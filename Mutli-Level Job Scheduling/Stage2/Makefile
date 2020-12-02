# run makefile
# $ make
# $ ./mlq random_list_jobs
# $ ./mlq random_list_jobs2


CC=gcc
CFLAGS=-D_POSIX_C_SOURCE -Wall -std=c11 -g -pthread
DEPS=mlq.o pcb.o mab.o
TARGET=mlq
PROCESS=process

all: $(TARGET) $(PROCESS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mlq: $(TAERGET)
	$(CC) -o $@ pcb.c mab.c mlq.c $(CFLAGS)

process: $(PROCESS)
	$(CC) -o $@ sigtrap.c

clean:
		rm -f *.o
		rm -f mlq
