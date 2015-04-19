all:
	mkdir -p ../backup/A
	mkdir -p ../backup/B
	mkdir -p ../backup/C
	mkdir -p ../backup/D
	mkdir -p ../backup/E
	mkdir -p ../mountdir
	gcc -Wall -std=c99 pfs.c log.c `pkg-config fuse --cflags --libs` -o pfs

clean:
	rm -f pfs
