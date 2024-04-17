CC = gcc
CFLAGS = -std=gnu99 -Wpedantic -pthread

all: gol_threads

gol_threads.o: gol_threads.c 
	$(CC) $(CFLAGS) -c gol_threads.c -o gol_threads.o

gol_threads: gol_threads.o
	$(CC) $(CFLAGS) gol_threads.o -o gol_threads

clean:
	rm gol_threads *.o