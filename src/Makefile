all:
	mkdir -p ../rootdir
	mkdir -p ../mountdir
	gcc -Wall pfs.c `pkg-config fuse --cflags --libs` -o pfs

clean:
	rm -f pfs
