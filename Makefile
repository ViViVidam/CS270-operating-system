CC = gcc

makenode:
	$(CC) -Wall SBFS.c nodes.c disk.c -o SBFS -lm
	./SBFS
	rm SBFS

#makefuse:
#	$(CC) -Wall main.c `pkg-config fuse3 --cflags --libs` -o main
#	./main -f mount
#	rm main

clean:
	rm main