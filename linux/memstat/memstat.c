/*  -*- mode: c; c-file-style: "LINUX"; -*-
 * Loadable module to replace the stock console beep routine
 *
 * Requires patches to .../linux/drivers/char/vt_kern.h and
 * .../linux/drivers/char/vt.c
 *
 * dave madden <dhm@proteon.com>
 */

#undef SHOW_BUFFERS

/* Kernel includes */
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <asm/page.h>
#include <linux/proc_fs.h>

#ifdef SHOW_BUFFERS
#include <linux/fs.h>
#include <linux/pagemap.h>
#endif

#ifdef SHOW_BUFFERS
#define NUMFIELDS 7
#else
#define NUMFIELDS 5
#endif

/* 4K page size but our output routines use some slack for overruns */
#define PROC_BLOCK_SIZE	(3*1024)  


static int memstat_read(struct inode * inode, struct file * file,char * buf, 
        int count);
int memstat_get_info(char *buffer, char **start, off_t offset, int length, 
        int dummy);

/* The string labels for the contents of memstat */
char LABELS[NUMFIELDS][20] = {
        "Total:",
        "Used:",
        "Free:",
        "1Count:",
        "Shared:"
#ifdef SHOW_BUFFERS
        , "Buffers:",
        "Cache:"
#endif
};

/* Structure for passing the values from si_memstat */
typedef struct {
        unsigned int total;
        unsigned int used;
        unsigned int free;
        unsigned int one_count;
        unsigned int shared;
#ifdef SHOW_BUFFERS
        unsigned int buffers;
        unsigned int cache;
#endif
} memstat;


static struct file_operations proc_memstat_operations = {
	NULL,
	memstat_read,
	NULL,
	NULL,		/* mem_readdir */
	NULL,		/* mem_select */
	NULL,		/* mem_ioctl */
	NULL,		/* mmap */
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* can't fsync */
};

struct inode_operations proc_memstat_inode_operations = {
	&proc_memstat_operations,	/* default base directory file-ops */
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL			/* permission */
};

static struct proc_dir_entry memstat_proc_dir_entry = {
        4000, 7, "memstat",
        S_IFREG | S_IRUGO, 1, 0, 0,
        0, &proc_memstat_inode_operations, 
        memstat_get_info, NULL, 
        NULL,  
        NULL, NULL
};

/* Construct the values from global kernel variables */
void si_memstat(memstat *val)
{
        int i;
        unsigned int *tmp;

#ifdef SHOW_BUFFERS
        val->cache = page_cache_size * PAGE_SIZE;
        val->buffers =  buffermem;
#endif

        val->total = val->used = val->free = val->one_count 
                = val->shared = 0;

        /* Loop through all pages tallying data */
        i = high_memory >> PAGE_SHIFT;
        while (i-- > 0)  {
                if (PageReserved(mem_map+i))
                        continue;

                val->total++;
                if (mem_map[i].count) {
                        val->used++;
                        if (mem_map[i].count > 1)
                                val->shared++;
                        else
                                val->one_count++;
                }
                else
                        val->free++;
        }
        
        for (i = 0, tmp = (unsigned int *)val ; i < NUMFIELDS ; i++, tmp++) {
                *tmp <<= PAGE_SHIFT;
                *tmp /= 1024;
        }

        return;
}



/*
 * put si_memstat into a char buffer in the way /proc wants it
 * To be honest, this is copied almost verbatim from the network
 * code.
 */

int memstat_get_info(char *buffer, char **start, off_t offset, int length, 
        int dummy)
{
        int len,size;
        off_t pos, begin;
        memstat Pages;
        unsigned int *p, i;

        len = pos = begin = 0;
        size = 0; /* sprintf(buffer, "-- Memory information --\n"); */
 
        si_memstat(&Pages);
        p = (unsigned int *) &Pages;
 
        pos += size;
        len += size;
 
        /* Add Pages to buffer */
        /* Add only how much is needed (length), but if offset is 0 */
        /* we have to start offset bytes into the output.  This     */
        /* approach re-does everything, discarding (copying over)   */
        /* characters until it gets up to offset                    */
        for (i=0; i < NUMFIELDS; i++) {
                size = sprintf(buffer+len, "%s %d\n", LABELS[i], p[i]);
                len += size;
                pos = begin+len;
                
                if (pos < offset) {
                        len = 0;
                        begin = pos;
                }
                if (pos > offset + length)
                        break;
        }
 
 
        /* calculate number of bytes added */
        *start = buffer + (offset - begin);
        len -= (offset - begin);
        if (len > length)
                len = length;
 
        return len;
}

static int memstat_read(struct inode * inode, struct file * file,char * buf, 
        int count)
{
	char * page;
	int bytes=count;
	int copied=0;
	char *start;
	struct proc_dir_entry * dp;
        
	if (count < 0)
		return -EINVAL;
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	if (!(page = (char*) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;

	while (bytes>0) {
		int length, thistime=bytes;
		if (bytes > PROC_BLOCK_SIZE)
			thistime=PROC_BLOCK_SIZE;

		length = dp->get_info(page, &start,
                        file->f_pos,
                        thistime,
                        (file->f_flags & O_ACCMODE) == O_RDWR);

		/*
 		 *	We have been given a non page aligned block of
		 *	the data we asked for + a bit. We have been given
 		 *	the start pointer and we know the length.. 
		 */

		if (length <= 0)
			break;
		/*
 		 *	Copy the bytes
		 */
		memcpy_tofs(buf+copied, start, length);
		file->f_pos += length;	/* Move down the file */
		bytes  -= length;
		copied += length;
		if (length<thistime)
			break;	/* End of file */
	}
	free_page((unsigned long) page);
	return copied;
}





/* required procedure called upon module initialization */
/* Installs the /proc handler */
int init_module( void )
{
        /* MOD_INC_USE_COUNT; */
	proc_register(&proc_root, &memstat_proc_dir_entry);

	return 0;
}

/* required procedure called upon module removal */
/* Removes the /proc handler */
void cleanup_module( void )
{
        proc_unregister(&proc_root,4000);
        /* MOD_DEC_USE_COUNT; */
}
