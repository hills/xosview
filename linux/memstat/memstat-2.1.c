#undef SHOW_BUFFERS

/* Kernel includes */
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <asm/page.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

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


static ssize_t memstat_read(struct file *, char *, size_t , loff_t *);
int memstat_get_info(char *);

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
	NULL,		/* mem_lseek */
	memstat_read, 
	NULL,i		/* mem_write */
	NULL,		/* mem_readdir */
	NULL,		/* mem_poll */
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
	0, &proc_memstat_inode_operations 
};

/* Construct the values from global kernel variables */
void si_memstat(memstat *val)
{
	int i;
	unsigned int *tmp;

#ifdef SHOW_BUFFERS
	val->cache = page_cache_size * PAGE_SIZE;
	val->buffers = buffermem;
#endif

	val->total = val->used = val->free = val->one_count 
		= val->shared = 0;

	/* Loop through all pages tallying data */
	i = max_mapnr;
	while (i-- > 0) {
		if (PageReserved(mem_map+i))
			continue;

		val->total++;
		if (atomic_read(&mem_map[i].count)) {
			val->used++;
			if (atomic_read(&mem_map[i].count) > 1)
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

int memstat_get_info(char *page)
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
	/* we have to start offset bytes into the output. This      */
	/* approach re-does everything, discarding (copying over)   */
	/* characters until it gets up to offset		    */
	for (i=0; i < NUMFIELDS; i++) {
		size = sprintf(page+len, "%s %d\n", LABELS[i], p[i]);
		len += size;
		
	}
 
	return len;
}

static ssize_t memstat_read(struct file * file, char * buf, 
	size_t count, loff_t *ppos)
{
	struct inode * inode = file->f_dentry->d_inode;
	unsigned long page;
	char *start;
	ssize_t length;
	ssize_t end;
	struct proc_dir_entry * dp;
	
	if (count > PROC_BLOCK_SIZE)
		count = PROC_BLOCK_SIZE;
	if (count < 0)
		return -EINVAL;
	if (!(page = __get_free_page(GFP_KERNEL)))
		return -ENOMEM;
	start = NULL;
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	if (dp->get_info)
		length = dp->get_info((char *)page, &start, *ppos,
				      count, 0);
	else
		length = memstat_get_info((char *) page);
	if (length < 0) {
		free_page(page);
		return length;
	}
	if (start != NULL) {
		/* We have had block-adjusting processing! */
		copy_to_user(buf, start, length);
		*ppos += length;
		count = length;
	} else {
		/* Static 4kB (or whatever) block capacity */
		if (*ppos >= length) {
			free_page(page);
			return 0;
		}
		if (count + *ppos > length)
			count = length - *ppos;
		end = count + *ppos;
		copy_to_user(buf, (char *) page + *ppos, count);
		*ppos = end;
	}
	free_page(page);
	return count;
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
