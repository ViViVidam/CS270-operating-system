CC = gcc

run:  disk.c nodes.c SBFS.c sbFuse.c
	$(CC) -Wall disk.c nodes.c SBFS.c sbFuse.c -lm -std=c++11 `pkg-config fuse3 --cflags --libs` -o main
	./main mount
	echo "hello world!" > mount/hello

debug: disk.c nodes.c SBFS.c sbFuse.c
	$(CC) -Wall disk.c nodes.c SBFS.c sbFuse.c -lm -std=c11 `pkg-config fuse3 --cflags --libs` -o main
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
	fusermount -u mount
	rm main
