#
# Makefile for the linux romfs filesystem routines.
#
FLAGS = -O3

inode.o:
	gcc $(FLAGS) -c inode.c
huffman.o:
	gcc $(FLAGS) -c compression/huffman/huffman.c
huflocal.o:
	gcc $(FLAGS) -c compression/huffman/huflocal.c
lz.o:
	gcc $(FLAGS) -c compression/lz/lz.c

CONFIG_MODULE=m
obj-$(CONFIG_MODULE) := zstfs.o
zstfs-objs := inode.o compression/huffman/huffman.o compression/huffman/huflocal.o compression/lz/lz.o

clean:
	rm -f *.o *.ko *.mod.* .*.cmd Module.symvers
	rm -rf .tmp_versions

