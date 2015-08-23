/*
 * ZSTFS file system, Linux implementation
 *
 * Copyright (C) 2008  
 * 
 * Danilo Tomasoni <ciropom at gmail dot com>
 * Martino Salvetti <the9ull at silix dot org>
 * Denisa Ziu <denisweet86 at yahoo dot it>
 *
 * Motto:

 * La teoria e` quando si sa tutto ma non funziona niente.
 * La pratica e` quando funziona tutto ma non si sa il perche`.
 * In ogni caso si finisce sempre con il coniugare la teoria con la pratica: non funziona niente e non si sa il perche`.

 * Albert Einstein

 * Using parts of the minix filesystem
 * Copyright (C) 1991, 1992  Linus Torvalds
 *
 * and parts of the affs filesystem additionally
 * Copyright (C) 1993  Ray Burr
 * Copyright (C) 1996  Hans-Joachim Widmaier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


/*
 * Sorry about some optimizations and for some goto's.  I just wanted
 * to squeeze some more bytes out of this code.. :)
 */

/* Compression Type
 *  1 -> obsoletecompress - test only
 *  2 -> bz2 - or something near it
 *  3 -> zlib
 *  4 ...
 */

/* Kernel 2.6.11.11 */
#define OLDKERNEL_

#define BWT_ENABLED_ /* bwt doesn't work */
#define BZ2_ENABLED_ /* not yet implemented */
#define ZLI_ENABLED_
#define OBS_ENABLED /* test only */
#define HUFF_ENABLED
#define ZST_ENABLED_
#define LZ_ENABLED

#ifdef LZ_ENABLED
#include "compression/lz/lz.h"
#else

typedef unsigned char byte;

#endif

#include "include.h"
#include "compression/huffman/huffman.h"

/* -- our *helper* functions -- */

#define CBSIZE(info) (1024<<((info>>20)&0xf))
#define CTYPE(info) ((info >> 24) & 0xff)

/* -- END -- */
/* compression function firm  */
#ifdef OBS_ENABLED
void decompress_obsolete( void *c, unsigned sc, void *dest );
#endif
#ifdef BWT_ENABLED
void bwt_decode(byte *buf_in, byte *buf_out, int size, int primary_index);
void bwt_decompress( void *c, unsigned sc, void *dest, unsigned sd );
#endif
#ifdef ZLI_ENABLED
int zlib_decompress( void *c, unsigned sc, void *dest, unsigned sd );
#endif
#ifdef ZST_ENABLED
void zstdecompress( void *c, unsigned sc, void *dest );
#endif

/* end */
struct romfs_inode_info {
  unsigned long i_metasize;	/* size of non-data area */
  /* used only for hard links and it's zero (i think) */
  unsigned long i_dataoffset;	/* from the start of fs */
  struct inode vfs_inode;
  int *index; /* dynamic array */
};

/* into private superblock data */
static inline unsigned long romfs_maxsize(struct super_block *sb)
{
   struct zst_sb_private *sb_p = (struct zst_sb_private *)sb->s_fs_info;
   return (unsigned long) sb_p->size;
}
/* get romfs inode from inode structure (in vfs.h) */
static inline struct romfs_inode_info *ROMFS_I(struct inode *inode)
{
   return container_of(inode, struct romfs_inode_info, vfs_inode);
}
/*
static __u32
romfs_checksum(void *data, int size)
{
   __u32 sum;
   __be32 *ptr;

   sum = 0;
   ptr = data;
   size>>=2;
   while (size>0) {
      sum += be32_to_cpu(*ptr++);
      size--;
   }
   return sum;
}
*/

static const struct super_operations romfs_ops;

static int romfs_fill_super(struct super_block *s, void *data, int silent)
{
   /* save in sb ctype e cbsize 
    */
   struct buffer_head *bh;
   struct romfs_super_block *rsb;
   struct inode *root;
   int sz;
   struct zst_sb_private *sb_p;


   /* I would parse the options here, but there are none.. :) */
   /* ROMBSIZE = 1024 */
   sb_set_blocksize(s, ROMBSIZE);
   s->s_maxbytes = 0xFFFFFFFF;

   bh = sb_bread(s, 0);
   if (!bh) {
      /* XXX merge with other printk? */
      printk ("romfs: unable to read superblock\n");
      return -EINVAL;
   }

   rsb = (struct romfs_super_block *)bh->b_data;
   sz = be32_to_cpu(rsb->size);
   /* ROMFH_SIZE -> file system header size*/
   if (rsb->word0 != ROMSB_WORD0 || sz < ROMFH_SIZE) {
      if (!silent)
         printk ("VFS: Can't find a romfs filesystem on dev "
                 "%s.\n", s->s_id);
      goto out;
   }
   /* checksum disabled
   if (romfs_checksum(rsb, min_t(int, sz, 512))) {
       printk ("romfs: bad initial checksum on dev "
               "%s.\n", s->s_id);
       goto out;
   }*/

   s->s_magic = ROMFS_MAGIC;
   /* s_fs_info -> *private date*
    *   
    */
   sb_p = (struct zst_sb_private *) kmalloc(sizeof(struct zst_sb_private),GFP_KERNEL);
   /* on umount we *should* free this memory
    */
   sb_p->size = (long)sz;
   sb_p->cbsize = CBSIZE(be32_to_cpu(rsb->info));
   sb_p->ctype = CTYPE(be32_to_cpu(rsb->info));

   /*printk("ZST: info=%d ctype=%d\n",be32_to_cpu(rsb->info),sb_p->ctype);*/

   s->s_fs_info = (void *)sb_p;

   s->s_flags |= MS_RDONLY;

   /* ROMFS_MAXFN = 128 */
   /* Find the start of the fs */
   sz = (ROMFH_SIZE +
         strnlen(rsb->name, ROMFS_MAXFN) + 1 + ROMFH_PAD)
        & ROMFH_MASK;

   s->s_op	= &romfs_ops;
   root = iget(s, sz);
   if (!root)
      goto out;

   s->s_root = d_alloc_root(root);
   if (!s->s_root)
      goto outiput;

   brelse(bh);
   return 0;

outiput:
   iput(root);
out:
   brelse(bh);

   return -EINVAL;
}

/* That's simple too. */

#ifdef OLDKERNEL
static int
romfs_statfs(struct super_block *sb, struct kstatfs *buf)
{
	buf->f_type = ROMFS_MAGIC;
	buf->f_bsize = ROMBSIZE;
	buf->f_bfree = buf->f_bavail = buf->f_ffree;
	buf->f_blocks = (romfs_maxsize(sb)+ROMBSIZE-1)>>ROMBSBITS;
	buf->f_namelen = ROMFS_MAXFN;
	return 0;
}
#else
static int
romfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
   /*non serve a niente*/
   buf->f_type = ROMFS_MAGIC;
   buf->f_bsize = ROMBSIZE;
   buf->f_bfree = buf->f_bavail = buf->f_ffree;
   buf->f_blocks = (romfs_maxsize(dentry->d_sb)+ROMBSIZE-1)>>ROMBSBITS;
   buf->f_namelen = ROMFS_MAXFN;
   return 0;
}
#endif


/* some helper routines */

static int
romfs_strnlen(struct inode *i, unsigned long offset, unsigned long count)
{
   struct buffer_head *bh;
   unsigned long avail, maxsize, res;

   maxsize = romfs_maxsize(i->i_sb);
   if (offset >= maxsize)
      return -1;

   /* strnlen is almost always valid */
   /* a che serve mettere di nuovo count > maxsize, se lo era ritornava -1 prima
      if (count > maxsize || offset+count > maxsize) */
   if ( offset+count > maxsize )
      count = maxsize-offset;

   bh = sb_bread(i->i_sb, offset>>ROMBSBITS);
   if (!bh)
      return -1;		/* error */

   avail = ROMBSIZE - (offset & ROMBMASK);
   maxsize = min_t(unsigned long, count, avail);
   res = strnlen(((char *)bh->b_data)+(offset&ROMBMASK), maxsize);
   brelse(bh);

   if (res < maxsize)
      return res;		/* found all of it */

   while (res < count) {
      offset += maxsize;

      bh = sb_bread(i->i_sb, offset>>ROMBSBITS);
      if (!bh)
         return -1;
      /* ritorna il minimo fra i due valori, il tipo e` passato tanto per sport */
      maxsize = min_t(unsigned long, count - res, ROMBSIZE);
      avail = strnlen(bh->b_data, maxsize);
      res += avail;
      brelse(bh);
      if (avail < maxsize)
         return res;
   }
   return res;
}

static int
romfs_copyfrom(struct inode *i, void *dest, unsigned long offset, unsigned long count)
{
   struct buffer_head *bh;
   unsigned long avail, maxsize, res;

   maxsize = romfs_maxsize(i->i_sb);
   if (offset >= maxsize || count > maxsize || offset+count>maxsize)
      return -1;

   bh = sb_bread(i->i_sb, offset>>ROMBSBITS); /*  10  */
   if (!bh)
      return -1;		/* error */

   avail = ROMBSIZE - (offset & ROMBMASK); /* 1023 == 111111111 (9 times) */
   maxsize = min_t(unsigned long, count, avail);
   memcpy(dest, ((char *)bh->b_data) + (offset & ROMBMASK), maxsize);
   brelse(bh);

   res = maxsize;			/* all of it */

   while (res < count) {
      offset += maxsize;
      dest += maxsize;

      bh = sb_bread(i->i_sb, offset>>ROMBSBITS); /* == 10 */
      if (!bh)
         return -1;
      maxsize = min_t(unsigned long, count - res, ROMBSIZE);
      memcpy(dest, bh->b_data, maxsize);
      brelse(bh);
      res += maxsize;
   }
   return res;
}

static unsigned char romfs_dtype_table[] = {
   DT_UNKNOWN, DT_DIR, DT_REG, DT_LNK, DT_BLK, DT_CHR, DT_SOCK, DT_FIFO
};

static int
romfs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
#ifdef OLDKERNEL
  struct inode *i = filp->f_dentry->d_inode; 
#else
  struct inode *i = filp->f_path.dentry->d_inode;
#endif
   struct romfs_inode ri;
   unsigned long offset, maxoff;
   int j, ino, nextfh;
   int stored = 0;
   char fsname[ROMFS_MAXFN];	/* XXX dynamic? */

   /*printk("ZST: romfs_readdir\n");*/

   lock_kernel();

   maxoff = romfs_maxsize(i->i_sb);

   /*loff_t f_pos; in struct file, loff_t type is set as long long*/
   /* it rapresent the position of the file */
   offset = filp->f_pos;

   if (!offset) {
      offset = i->i_ino & ROMFH_MASK;
      if (romfs_copyfrom(i, &ri, offset, ROMFH_SIZE) <= 0)
         goto out;
      offset = be32_to_cpu(ri.spec) & ROMFH_MASK;
   }

   /* Not really failsafe, but we are read-only... */
   for (;;) {
      if (!offset || offset >= maxoff) {
         offset = maxoff;
         filp->f_pos = offset;
         goto out;
      }
      filp->f_pos = offset;

      /* Fetch inode info */
      if (romfs_copyfrom(i, &ri, offset, ROMFH_SIZE) <= 0)
         goto out;

      j = romfs_strnlen(i, offset+ROMFH_SIZE, sizeof(fsname)-1);
      if (j < 0)
         goto out;

      fsname[j]=0;
      romfs_copyfrom(i, fsname, offset+ROMFH_SIZE, j);

      ino = offset;
      nextfh = be32_to_cpu(ri.next);
      if ((nextfh & ROMFH_TYPE) == ROMFH_HRD)
         ino = be32_to_cpu(ri.spec);
      if (filldir(dirent, fsname, j, offset, ino,
                  romfs_dtype_table[nextfh & ROMFH_TYPE]) < 0) {
         goto out;
      }
      stored++;
      offset = nextfh & ROMFH_MASK;
   }
out:
   unlock_kernel();
   return stored;
}

static struct dentry *
         romfs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd)
{
   unsigned long offset, maxoff;
   int fslen, res;
   struct inode *inode;
   char fsname[ROMFS_MAXFN];	/* XXX dynamic? */
   struct romfs_inode ri;
   const char *name;		/* got from dentry */
   int len;

   res = -EACCES;			/* placeholder for "no data here" */
   offset = dir->i_ino & ROMFH_MASK;
   lock_kernel();
   if (romfs_copyfrom(dir, &ri, offset, ROMFH_SIZE) <= 0)
      goto out;

   maxoff = romfs_maxsize(dir->i_sb);
   offset = be32_to_cpu(ri.spec) & ROMFH_MASK;

   /* OK, now find the file whose name is in "dentry" in the
    * directory specified by "dir".  */

   name = dentry->d_name.name;
   len = dentry->d_name.len;

   for (;;) {
      if (!offset || offset >= maxoff)
         goto out0;
      if (romfs_copyfrom(dir, &ri, offset, ROMFH_SIZE) <= 0)
         goto out;

      /* try to match the first 16 bytes of name */
      fslen = romfs_strnlen(dir, offset+ROMFH_SIZE, ROMFH_SIZE);
      if (len < ROMFH_SIZE) {
         if (len == fslen) {
            /* both are shorter, and same size */
            romfs_copyfrom(dir, fsname, offset+ROMFH_SIZE, len+1);
            if (strncmp (name, fsname, len) == 0)
               break;
         }
      } else if (fslen >= ROMFH_SIZE) {
         /* both are longer; XXX optimize max size */
         fslen = romfs_strnlen(dir, offset+ROMFH_SIZE, sizeof(fsname)-1);
         if (len == fslen) {
            romfs_copyfrom(dir, fsname, offset+ROMFH_SIZE, len+1);
            if (strncmp(name, fsname, len) == 0)
               break;
         }
      }
      /* next entry */
      offset = be32_to_cpu(ri.next) & ROMFH_MASK;
   }

   /* Hard link handling */
   if ((be32_to_cpu(ri.next) & ROMFH_TYPE) == ROMFH_HRD)
      offset = be32_to_cpu(ri.spec) & ROMFH_MASK;

   if ((inode = iget(dir->i_sb, offset)))
      goto outi;

   /*
    * it's a bit funky, _lookup needs to return an error code
    * (negative) or a NULL, both as a dentry.  ENOENT should not
    * be returned, instead we need to create a negative dentry by
    * d_add(dentry, NULL); and return 0 as no error.
    * (Although as I see, it only matters on writable file
    * systems).
    */

out0:
   inode = NULL;
outi:
   res = 0;
   d_add (dentry, inode);

out:
   unlock_kernel();
   return ERR_PTR(res);
}
/* decompression algorithm implementation */

#ifdef OBS_ENABLED
void decompress_obsolete( void *c, unsigned sc, void *dest ){
  /*
    def obsoletecompress(data):
    return '[BEGIN BLOCK>'+data+'<END BLOCK]'
    13, 11
  */
  /*printk("ZST decompress: dim data=%d\n",sc-13-11);*/
  memcpy( dest, c+13 , sc-13-11 );
}
#endif

#ifdef BWT_ENABLED
void bwt_decode(byte *buf_in, byte *buf_out, int size, int primary_index)
{
    byte F[size];
    int buckets[256];
    int i,j,k;
    int indices[size];
 
    for (i=0; i<256; i++)
        buckets[i] = 0;
    for (i=0; i<size; i++)
        buckets[buf_in[i]] ++;
    for (i=0,k=0; i<256; i++)
        for (j=0; j<buckets[i]; j++)
            F[k++] = i;
    
    /*assert (k==size);*/
    
    for (i=0,j=0; i<256; i++)
    {
        while (i>F[j] && j<size)
            j++;
        buckets[i] = j; /* it will get fake values if there is no i in F, but
                           that won't bring us any problems */
    }
    for(i=0; i<size; i++)
        indices[buckets[buf_in[i]]++] = i;
    for(i=0,j=primary_index; i<size; i++)
    {
        buf_out[i] = buf_in[j];
        j=indices[j];
    }
}

void bwt_decompress( void *c, unsigned sc, void *dest, unsigned sd ){
  /* the first 4 byte is the primary_index */
  int primary_index;
  int i,j=0;
  char x;
  byte pdc[sd]; /* contains partially decompressed block */
  byte *rc = (byte *) c+4; /* real compressed buffer, cut the primary index */

  /* save primary index */
  memcpy( &primary_index, c, 4 );
  primary_index = be32_to_cpu( primary_index );
 
  /* read 2 byte. The first was the real byte, the second represents the number of times in which
     the first byte appears in the block */
  for( i = 0 ; i < sc ; i+=2 )
    /* the number of occorrences of byte (int) *( rc + i + 1 )+1 
       (because if there are 1 occurrence the number is 0) */
    /* fill the partially decompressed buffer */
    for( x = 0 ; x < (*( rc + i + 1 )+1) ; ++x )
      pdc[j++] = *( rc + i );
  
  
  /* complete decode, and save in destination buffer */
  bwt_decode( pdc, (byte *) dest, sd, primary_index );

}
#endif
#ifdef ZLI_ENABLED
int zlib_decompress( void *c, unsigned sc, void *dest, unsigned sd ){
  z_stream strm;
  
  /* fill strm */
  strm.next_in = (unsigned char *) c;
  strm.avail_in = sc;
  strm.total_in = sc;
  
  strm.next_out = (unsigned char *) dest;
  strm.avail_out = sd;
  strm.total_out = sd;

  strm.data_type = Z_UNKNOWN; /* FOR NOW */
  
  /* non conosce qualcosa qui... le costanti sono interi normalissimi O.o */
  return zlib_inflate( &strm, Z_SYNC_FLUSH ) == Z_OK; 

  /*  int inf(FILE *source, FILE *dest) */
  /*
  int ret;
  z_stream strm;
  char *in = (char *) c;
  char *out = (char *) dest;

    strm.avail_in = 0;
    strm.next_in = 0;
    
    do {
        strm.avail_in = sc;
        
	if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        do {
            strm.avail_out = sd;
            strm.next_out = out;
            ret = zlib_inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  
            switch (ret) {
            case Z_NEED_DICT:
	      ret = Z_DATA_ERROR;     
            case Z_DATA_ERROR:
	      printk("dataerror\n");
            case Z_MEM_ERROR:
	      printk("memerror\n" );
	      zlib_inflateEnd(&strm);
	      return ret;
            }
            
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     

    } while (ret != Z_STREAM_END);

    zlib_inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
  */  
}
#endif
#ifdef ZST_ENABLED

void zstdecompress( void *c, unsigned sc, void *dest ){
  unsigned i;
  unsigned char *buf = (char *)c;
  unsigned char nbyte = *buf;
  unsigned short header[nbyte][2];
  unsigned char pointer, no; /* pointer to block, number of occorrences */

  if( !nbyte ){
    memcpy( dest, c+1, sc-1 );
    return;
  }

  /* read header of block*/
  for( i = 1 ; i < nbyte-1 ; i+=3 ){
    header[i-1][0] = buf[i+1];
    header[i-1][0] = header[i-1][0] | (((short) buf[i]) << 8) ;
    header[i-1][1] = buf[i+2];
  }

  for( i = nbyte+1 ; i < sc ; ++i ){
    
  }


}

#endif
/*
 * Ok, we do readpage, to be able to execute programs.  Unfortunately,
 * we can't use bmap, since we may have looser alignments.
 */

static int
romfs_readpage(struct file *file, struct page * page)
{
   /* this function read the necessary block, decompress it, and return the 
      data requested from offset.
   */
   struct inode *inode = page->mapping->host;
   /* this should be loff_t, but insmod doesn't work with this type */
   int offset, avail, readlen;
   void *buf;
   void *cbuf, *dbuf; /* compressed buf, decompressed buf */
   int result = -EIO;
   unsigned ib,fb,i; /* initial block , final block */
   int *index = ROMFS_I(inode)->index; /* index of compressed block */
   int cbsize = ((struct zst_sb_private *)inode->i_sb->s_fs_info)->cbsize;
#ifdef LZ_ENABLED
   int read;
#endif

   page_cache_get(page);
   lock_kernel();
   buf = kmap(page);
   if (!buf)
      goto err_out;

   /* 32 bit warning -- but not for us :) */
   offset = page_offset(page);
   
   /* if there is 0 byte to read */
   if(offset == i_size_read(inode)) {
     memset(buf, 0, PAGE_SIZE);
     SetPageUptodate(page);
     result = 0;
   }
   else if (offset < i_size_read(inode)) {
     
     avail = inode->i_size-offset;
     readlen = min_t(unsigned long, avail, PAGE_SIZE);
     /* initial block id */
     ib = (unsigned) offset / cbsize;
     /* add another block to final block id only if necessary */
     if( (offset + readlen) % cbsize )
       fb = ((unsigned) (offset + readlen) / cbsize)+1;
     else
       fb = ((unsigned) (offset + readlen) / cbsize);
     /* buffer for store the compressed block */  
     cbuf = kmalloc( index[fb] - index[ib], GFP_KERNEL );
     
     /* alloc temporary memory to store the decompressed data */
     dbuf = kmalloc( cbsize * (fb-ib),GFP_KERNEL);
    
     if(!cbuf || !dbuf)
       goto err_out;
     /* copy in temporary cbuf the necessary block, if read all the data continue, else error */
     if (romfs_copyfrom(inode, cbuf, index[ib] , index[fb]-index[ib]) == index[fb]-index[ib] ) {

       /* the decompression must be done for each block */
       for( i = ib; i < fb ; ++i )
	 switch( ((struct zst_sb_private *)inode->i_sb->s_fs_info)->ctype ){
#ifdef LZ_ENABLED
	 case COMP_LZ:
	   if( ( read = Decompression( (ubyte *) (cbuf + index[i] - index[ib]) ,  
				       (ubyte *) (dbuf + (i - ib)*cbsize) ,
				       (uword) (index[i+1] - index[i])
				       )) != cbsize ){
	     
	     printk("ZST ERROR: decompression failed! decompressed %d byte instead of %d\n",read,cbsize  );
	     /*
	     *((ubyte *) ( dbuf + (i - ib)*cbsize + read )) = '\0';
	     
	     printk("ZST DATA READ:\n%s\n", (char *) dbuf + (i - ib)*cbsize );
	     
	     goto err_out;
	     */
	     }
	     
#endif
#ifdef OBS_ENABLED
	 case COMP_OBS:
	   
	   /*  
	       pointer to the begin of compressed current block, 
	       size of compressed block,
	       pointer to free memory of buffer
	       
	    */
	   decompress_obsolete( cbuf + index[i] - index[ib] , 
				index[i+1] - index[i], 
				dbuf + (i - ib)*cbsize 
				);
	   break;
#endif
#ifdef BWT_ENABLED
	 case COMP_BWT:
	   bwt_decompress( cbuf + index[i] - index[ib] , 
			   index[i+1] - index[i], 
			   dbuf + (i - ib)*cbsize,
			   cbsize
			   );
	   break;
#endif
#ifdef BZ2_ENABLED
	 case COMP_BZ2:
	   printk("ZST: compression type not yet supported\n");
	   goto err_out;
	   break;
#endif
#ifdef ZLI_ENABLED
	 case COMP_ZLI:
	   printk( "ZST: zlib:\n" );
	   if( zlib_decompress( cbuf + index[i] - index[ib] ,
				index[i+1] - index[i],
				dbuf + (i-ib)*cbsize,
				cbsize ) != Z_OK ){
	     printk("zstfs: error in decompression!!\n");
	     goto err_out;
	   }
	   printk( "ZST: fine zlib\n" );
	   printk( "ZST: dati: %s\n", (char *) dbuf );
	   break;
#endif
#ifdef HUFF_ENABLED
	 case COMP_HUFF:

	   if( !HuffmanDecodeFile( cbuf + index[i] - index[ib] ,
				   index[i+1] - index[i],
				   dbuf + (i-ib)*cbsize,
				   cbsize ) ){
	     printk( "ZST: error in huffmandecoding!\n" );
	     goto err_out;
	   }
	     
	   break;
#endif
	 default:
	   printk("ZST: compression type not yet supported\n");
	   goto err_out;
	 }
       /* page, begin of page to be copied, dim of page to be copied  */
       memcpy( buf, dbuf + offset % cbsize , readlen );
       
       if (readlen < PAGE_SIZE) {
	 memset(buf + readlen,0,PAGE_SIZE-readlen);
       }
       SetPageUptodate(page);
       result = 0;
       
     }
     /* free temporary buffer */
     kfree( dbuf );
     kfree( cbuf );
   }
   
   if (result) {
     memset(buf, 0, PAGE_SIZE);
     SetPageError(page);
   }

   flush_dcache_page(page);
   
   unlock_page(page);
   
   kunmap(page);
err_out:
   page_cache_release(page);
   unlock_kernel();

   return result;
}

/* Mapping from our types to the kernel */

static const struct address_space_operations romfs_aops = {
  .readpage = romfs_readpage
};

static const struct file_operations romfs_dir_operations = {
  .read		= generic_read_dir,
  .readdir	= romfs_readdir,
};

static const struct inode_operations romfs_dir_inode_operations = {
  .lookup		= romfs_lookup,
};

static mode_t romfs_modemap[] =
{
   0, S_IFDIR+0644, S_IFREG+0644, S_IFLNK+0777,
   S_IFBLK+0600, S_IFCHR+0600, S_IFSOCK+0644, S_IFIFO+0644
};

static void
romfs_read_inode(struct inode *i)
{
  /* this function must read index and real size,
   * it would be stored in ROMFS_I(inode)
   */
  int nextfh, ino, size, *index, isize, a;
  struct romfs_inode ri;

  ino = i->i_ino & ROMFH_MASK;
  i->i_mode = 0;

  /* Loop for finding the real hard link */
  for (;;) {
    if (romfs_copyfrom(i, &ri, ino, ROMFH_SIZE) <= 0) {
      printk("zstfs: read error for inode 0x%x\n", ino);
      return;
    }
    /* XXX: do romfs_checksum here too (with name) */
    
    nextfh = be32_to_cpu(ri.next);
    if ((nextfh & ROMFH_TYPE) != ROMFH_HRD)
      break;
    
    /* ROMFH_MASK = 0xfffffff0 */
    ino = be32_to_cpu(ri.spec) & ROMFH_MASK;
  }

  
  i->i_nlink = 1;		/* Hard to decide.. */
  i->i_size = be32_to_cpu(ri.size);/* compressed size */
  i->i_mtime.tv_sec = i->i_atime.tv_sec = i->i_ctime.tv_sec = 0;
  i->i_mtime.tv_nsec = i->i_atime.tv_nsec = i->i_ctime.tv_nsec = 0;
  i->i_uid = i->i_gid = 0;

  /* Precalculate the data offset - not real data offset yet */
  ino = romfs_strnlen(i, ino+ROMFH_SIZE, ROMFS_MAXFN);
  if (ino >= 0)
    ino = ((ROMFH_SIZE+ino+1+ROMFH_PAD)&ROMFH_MASK);
  else
    ino = 0;
  
  ROMFS_I(i)->i_metasize = ino;
  ROMFS_I(i)->i_dataoffset = ino+(i->i_ino&ROMFH_MASK); /* point to u_size */
  ROMFS_I(i)->index = NULL; /* init index to 0, destroy_inode needs it */
  
  /* Compute permissions */
  ino = romfs_modemap[nextfh & ROMFH_TYPE];
  /* only "normal" files have ops */
  switch (nextfh & ROMFH_TYPE) {
  case 1:
    i->i_size = ROMFS_I(i)->i_metasize;
    i->i_op = &romfs_dir_inode_operations;
    i->i_fop = &romfs_dir_operations;
    if (nextfh & ROMFH_EXEC)
      ino |= S_IXUGO;
    i->i_mode = ino;
    break;
  case 2:
    i->i_fop = &generic_ro_fops;
    i->i_data.a_ops = &romfs_aops;
    if (nextfh & ROMFH_EXEC)
      ino |= S_IXUGO;
    i->i_mode = ino;

    
    /* read real size */
    romfs_copyfrom(i,&size,ROMFS_I(i)->i_dataoffset,4);
    /* convert to big endian */
    size = be32_to_cpu(size);
    
    a = ((struct zst_sb_private *)i->i_sb->s_fs_info)->cbsize;
    /* index size */
    isize = 1 + size / a;

    /* ceil */
    if(size%a)
      isize++;

    /* alloc index */
    index = (int *) kmalloc(sizeof(int)*isize,GFP_KERNEL);
    
    /* if isize == 2 -> no index on disk */
    if(isize > 2){
      /* the size of index on disk is isize-1 */
      if(romfs_copyfrom(i,index,ROMFS_I(i)->i_dataoffset+4,4*(isize-1))<4*(isize-1)){
	printk("ZST: i cannot read index\n");
      }
      /* convert index in big endian*/
      for(a=0 ; a<isize-1 ; a++)
	index[a] = be32_to_cpu(index[a]);
      /* (isize - 1 + 1 )*4 
	 -1 for the last index (that there isn't on hd)
	 +1 for the rsize
      */
      index[a] = index[0] + (i->i_size - isize*4);
    }
    else {
      index[0] = ROMFS_I(i)->i_dataoffset + 4; /* Beginning of data*/
      index[1] = index[0] + (i->i_size - 4); /* the is only realsize */
    }
    /*
      debug:
    for(a=0 ; a<isize ; a+=1000)
      printk("ZST:\t%d %d\n",a,index[a]);
    if(isize>5)
      for(a=isize-5 ; a<isize ; a++)
	printk("ZST:\t%d %d\n",a,index[a]);
    */
    i->i_size = size; /* uncompressed file size */
    ROMFS_I(i)->index = index; /* save index */

    break;
  case 3:
    i->i_op = &page_symlink_inode_operations;
    i->i_data.a_ops = &romfs_aops;
    i->i_mode = ino | S_IRWXUGO;
    break;
  default:
    /* depending on MBZ for sock/fifos */
    nextfh = be32_to_cpu(ri.spec);
    init_special_inode(i, ino,
		       MKDEV(nextfh>>16,nextfh&0xffff));
  }
}
#ifdef OLDKERNEL
static kmem_cache_t * romfs_inode_cachep;
#else
static struct kmem_cache * romfs_inode_cachep;
#endif
static struct inode *romfs_alloc_inode(struct super_block *sb)
{
   struct romfs_inode_info *ei;

   ei = kmem_cache_alloc(romfs_inode_cachep, GFP_KERNEL);
   if (!ei)
      return NULL;

   return &ei->vfs_inode;
}

static void romfs_destroy_inode(struct inode *inode)
{
  if(ROMFS_I(inode)->index) /* check if index exists */
    kfree(ROMFS_I(inode)->index); /* free index */
  kmem_cache_free(romfs_inode_cachep, ROMFS_I(inode));
}


#ifdef OLDKERNEL
static void init_once(void * foo, kmem_cache_t * cachep, unsigned long flags)
{
	struct romfs_inode_info *ei = (struct romfs_inode_info *) foo;

	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR)) ==
	    SLAB_CTOR_CONSTRUCTOR)
		inode_init_once(&ei->vfs_inode);
}
#else

static void init_once(struct kmem_cache *cachep, void *foo)
{
   struct romfs_inode_info *ei = foo;

   inode_init_once(&ei->vfs_inode);
}
#endif

static int init_inodecache(void)
{	

   romfs_inode_cachep = kmem_cache_create("zstfs_inode_cache",
                                          sizeof(struct romfs_inode_info),
                                          0, 
#ifdef OLDKERNEL 
					  SLAB_RECLAIM_ACCOUNT,
#else
					  (SLAB_RECLAIM_ACCOUNT|
                                              SLAB_MEM_SPREAD),
#endif
					  init_once
#ifdef OLDKERNEL
					  ,NULL
#endif
					  );

   if (romfs_inode_cachep == NULL)
      return -ENOMEM;
   return 0;
}

static void destroy_inodecache(void)
{

#ifdef OLDKERNEL
  if (kmem_cache_destroy(romfs_inode_cachep))
		printk(KERN_INFO "romfs_inode_cache: not all structures were freed\n");

#else
   kmem_cache_destroy(romfs_inode_cachep);
#endif

}

static int romfs_remount(struct super_block *sb, int *flags, char *data)
{
   *flags |= MS_RDONLY;
   return 0;
}

static const struct super_operations romfs_ops = {
  .alloc_inode	        = romfs_alloc_inode,
  .destroy_inode        = romfs_destroy_inode,
  .read_inode	        = romfs_read_inode,
  .statfs	        = romfs_statfs,
  .remount_fs	        = romfs_remount,
};
#ifdef OLDKERNEL

static struct super_block *romfs_get_sb(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data)
{
        return get_sb_bdev(fs_type, flags, dev_name, data, romfs_fill_super);
}

#else

static int romfs_get_sb(struct file_system_type *fs_type,
                        int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{

   return get_sb_bdev(fs_type, flags, dev_name, data, romfs_fill_super,
                      mnt);
}

#endif

void kill_block_super_zstfs(struct super_block *sb){
  kfree(sb->s_fs_info); /* free sb private info */
  kill_block_super(sb);
}

static struct file_system_type romfs_fs_type = {
  .owner	= THIS_MODULE,
  .name		= "zstfs",
  .get_sb	= romfs_get_sb,
  .kill_sb	= kill_block_super_zstfs, /* replaced default kill_block_super */
  .fs_flags	= FS_REQUIRES_DEV,
};

static int __init init_romfs_fs(void)
{
   int err = init_inodecache();
   if (err)
      goto out1;
   err = register_filesystem(&romfs_fs_type);
   if (err)
      goto out;
   return 0;
out:
   destroy_inodecache();
out1:
   return err;
}

static void __exit exit_romfs_fs(void)
{
   unregister_filesystem(&romfs_fs_type);
   destroy_inodecache();
}

/* Yes, works even as a module... :) */

module_init(init_romfs_fs)
module_exit(exit_romfs_fs)
MODULE_LICENSE("GPL");
