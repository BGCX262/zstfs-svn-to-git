#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include "zstfs_fs.h" /* we modified it */
#include <linux/zconf.h>
#include <linux/zlib.h> /* compression */
/* 
see http://www.bzip.org/1.0.5/bzip2-manual-1.0.5.html#top-level
#include "compression/bzip2/bzlib.h" 
*/
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/smp_lock.h>
#include <linux/buffer_head.h>
#include <linux/vfs.h>

#include <asm/uaccess.h>


#endif
