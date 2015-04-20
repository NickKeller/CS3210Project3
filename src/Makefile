all:
	gcc -Wall -std=c99 -fno-stack-protector pfs.c log.c database.c hash.c `mysql_config --cflags --libs` `pkg-config fuse --cflags --libs` -o pfs



clean:
	rm -f pfs
