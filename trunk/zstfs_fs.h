/*

La teoria e` quando si sa tutto ma non funziona niente.
La pratica e` quando funziona tutto ma non si sa il perche`.
In ogni caso si finisce sempre con il coniugare la teoria con la pratica: non funziona niente e non si sa il perche`.

Albert Einstein

*/

#ifndef __LINUX_ROMFS_FS_H
#define __LINUX_ROMFS_FS_H

/* The basic structures of the romfs filesystem */

#define ROMBSIZE BLOCK_SIZE /* 1<<BLOCK_SIZE_BITS == 1024  */
#define ROMBSBITS BLOCK_SIZE_BITS /* BLOCKSIZEBITS == 10 (include/linux/fs.h) */
#define ROMBMASK (ROMBSIZE-1) /* 1023 */
#define ROMFS_MAGIC 0x7275

#define ROMFS_MAXFN 128

#define __mkw(h,l) (((h)&0x00ff)<< 8|((l)&0x00ff))
#define __mkl(h,l) (((h)&0xffff)<<16|((l)&0xffff))
#define __mk4(a,b,c,d) cpu_to_be32(__mkl(__mkw(a,b),__mkw(c,d)))
#define ROMSB_WORD0 __mk4('z','s','t','0')
/*#define ROMSB_WORD1 __mk4('1','f','s','-')*/

/* On-disk "super block" */

struct romfs_super_block {
  __be32 word0;
  /* ex word1 */
  /* contains ctype and bsize */
  __be32 info;
  __be32 size;
  __be32 checksum;
  char name[0];		/* volume name */
};

/* On disk inode */

struct romfs_inode {
  __be32 next;		/* low 4 bits see ROMFH_ */
  __be32 spec;
  __be32 size;
  __be32 checksum;
  char name[0];
  /*__be32 rsize; / * real size: it should be integer (32 bit) */
  /*__be32 index[0]; / * array of index, every value in the array is a pointer to a block of the compressed file */
};

#define ROMFH_TYPE 7
#define ROMFH_HRD 0
#define ROMFH_DIR 1
#define ROMFH_REG 2
#define ROMFH_SYM 3
#define ROMFH_BLK 4
#define ROMFH_CHR 5
#define ROMFH_SCK 6
#define ROMFH_FIF 7
#define ROMFH_EXEC 8

/* Alignment */

#define ROMFH_SIZE 16
#define ROMFH_PAD (ROMFH_SIZE-1)
#define ROMFH_MASK (~ROMFH_PAD)

/* ZST structure(s) */
struct zst_sb_private {
  long size; /* uncompressed size */
  int cbsize;
  char ctype;
};

#define COMP_OBS 1
#define COMP_BZ2 2
#define COMP_ZLI 3
#define COMP_BWT 4 /* need to save 'primary_index'? */
#define COMP_HUFF 5
#define COMP_LZ 6

#ifdef __KERNEL__

/* Not much now */

#endif /* __KERNEL__ */
#endif
