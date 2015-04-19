all:
	gcc -Wall -std=c99 pfs.c log.c `pkg-config fuse --cflags --libs` -o pfs

clean:
	rm -f pfs
