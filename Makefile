CC = gcc

all: disk node SBFS
	$(CC) -Wall disk.o nodes.o SBFS.o -o main -lm
	make clean
	./main
	rm main

fuse: disk node SBFS
	$(CC) -Wall -c sbFuse.c -o sbFuse.o
	$(CC) -Wall sbFuse.o disk.o nodes.o SBFS.o `pkg-config fuse3 --cflags --libs` -o main
	./main -f mount

disk:
	$(CC) -Wall -c disk.c -o disk.o

node:
	$(CC) -Wall -c nodes.c -o nodes.o

SBFS:
	$(CC) -Wall -c SBFS.c -o SBFS.o

#makefuse:
#	$(CC) -Wall main.c `pkg-config fuse3 --cflags --libs` -o main
#	./main -f mount
#	rm main

clean:
	rm *.o