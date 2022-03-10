CC = gcc

run:  disk.c nodes.c SBFS.c sbFuse.c SBFSHelper.c sbfuseHelper.c
	$(CC) -Wall disk.c nodes.c SBFSHelper.c SBFS.c sbfuseHelper.c sbFuse.c -lm `pkg-config fuse3 --cflags --libs` -o main
	./main mount

gcc9:
	scl enable devtoolset-9 bash

debug: disk.c nodes.c SBFS.c sbFuse.c sbfuseHelper.c
	$(CC) -Wall disk.c nodes.c SBFSHelper.c SBFS.c sbfuseHelper.c sbFuse.c -lm `pkg-config fuse3 --cflags --libs` -o main
	./main -f mount

fuse:
	$(CC) -Wall disk.c nodes.c SBFSHelper.c SBFS.c -lm -o debug
	./debug
	rm debug

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
	fusermount -u mount
	rm main
