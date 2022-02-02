CC = gcc

all: disk node SBFS
	$(CC) -Wall disk.o nodes.o SBFS.o -o main -lm
	make clean
	./main
	rm main

run:  disk.c nodes.c SBFS.c sbFuse.c
	$(CC) -Wall disk.c nodes.c SBFS.c sbFuse.c -lm `pkg-config fuse3 --cflags --libs` -o main
	./main mount
	echo "int main(printf(\"hello world\\n\");){};" > mount/hello
	fusermount -u mount

fuse: disk.c nodes.c SBFS.c sbFuse.c
	$(CC) -Wall disk.c nodes.c SBFS.c sbFuse.c -lm `pkg-config fuse3 --cflags --libs` -o main
	./main -f mount
	echo "int main(printf(\"hello world\\n\");){};" > mount/hello
	fusermount -u mount

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