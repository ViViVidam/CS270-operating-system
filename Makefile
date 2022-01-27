CC = gcc

#makenode:
#	$(CC) -Wall nodes.c disk.c -o nodes -lm
#	./nodes
#	rm nodes

makefuse:
	$(CC) -Wall main.c `pkg-config fuse3 --cflags --libs` -o main
	./main -f mount 
	rm main