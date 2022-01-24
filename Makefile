CC = gcc
makefuse:
	$(CC) -Wall main.c `pkg-config fuse3 --cflags --libs` -o main
	./main -f mount
	rm main