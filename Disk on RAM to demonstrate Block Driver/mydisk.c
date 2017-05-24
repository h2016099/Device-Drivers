#include <linux/fs.h>				// register_blkdev, block_device_operations
#include <linux/genhd.h>			// gendisk structure
#include <linux/module.h>			// For module initialisation and exit
#include <linux/kernel.h>			// For kernel info,alerts
#include <linux/blkdev.h>			// request_queue
#include <linux/errno.h>			// Errors
#include <linux/bio.h>				// Bio structure
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/hdreg.h> 			// For struct hd_geometry
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/errno.h>
#include "partition.h"

#define KERNEL_SECTOR_SIZE 512			// SECTOR SIZE
#define NSECTORS 1024				// 1024*512 = 512KB
#define MYDISK_MINORS 8				// 3 - logical , 3 - primary
#define BV_PAGE(bv) ((bv).bv_page)
#define BV_OFFSET(bv) ((bv).bv_offset)
#define BV_LEN(bv) ((bv).bv_len)



static u_int mydisk_major = 0;
u8 *dev_data;

static struct mydisk_device
{
	int size;
	u8 *data;
	spinlock_t lock;
	struct request_queue *queue;
	struct gendisk *gd;
}mydisk_dev;

static int mydisk_open(struct block_device *bdev, fmode_t mode)
{
	printk(KERN_INFO "Mydisk: Device is opened\n");
	return 0;
}

static void mydisk_close(struct gendisk *disk, fmode_t mode)
{
	printk(KERN_INFO "Mydisk: Device is closed\n");
}



static int mydisk_transfer(struct request *rq)
{

	int dir = rq_data_dir(rq);
	sector_t start_sector = blk_rq_pos(rq);

	struct bio_vec bv;

	struct req_iterator iter;

	unsigned int sectors;
	u8 *buffer;

	int ret = 0;
	
	rq_for_each_segment(bv, rq, iter)
	{
	
		buffer = page_address(BV_PAGE(bv)) + BV_OFFSET(bv);

		if (BV_LEN(bv) %KERNEL_SECTOR_SIZE != 0)
		{
			ret = -EIO;
		}

		sectors = BV_LEN(bv) / KERNEL_SECTOR_SIZE;

		if (dir == WRITE) /* Write to the device */
		{
			memcpy(dev_data + (start_sector)*KERNEL_SECTOR_SIZE, buffer,sectors * KERNEL_SECTOR_SIZE);
		}
		else /* Read from the device */
		{
			memcpy(buffer, dev_data + (start_sector)*KERNEL_SECTOR_SIZE,sectors * KERNEL_SECTOR_SIZE);
		}

	}


	return 0;
}

void mydisk_request(struct request_queue *queue)
{

	struct request *req;

	while ((req = blk_fetch_request(queue)) != NULL) 
	{	
					

		if (!(req->cmd_type == REQ_TYPE_FS)) 	  	
		{

			// check whether the request that do not move blocks from disk, tells whether we are
			// looking at a file system request, if it is not then we pass it to end_request
			printk(KERN_NOTICE "Skip non-fs request\n");	
			__blk_end_request_all(req, -1);		// <0 for failure
			// Reference - https://return42.github.io/sphkerneldoc/linux_src_doc/block/blk-core_c.html#blk-end-request-cur
			continue;
		}

		mydisk_transfer(req); 	// To move data 
		
		__blk_end_request_all(req, 0);	// suceeded as 0 and neg for failure	

	}	

}

static struct block_device_operations mydisk_fops =
{
	.owner = THIS_MODULE,
	.open = mydisk_open,
	.release = mydisk_close,
};

// To make block devices operations available to the system by way of file_ops structure


static int __init mydisk_init(void)
{
	dev_data = vmalloc(NSECTORS * KERNEL_SECTOR_SIZE);	// ALLOCATING SIZE FOR BLOCK DEVICE DRIVER, Virtually contiguous Memory
	
	copy_mbr_n_br(dev_data);				// To setup its partition table

	mydisk_dev.size = NSECTORS;				// In Sectors - 512 bytes

			
	// Step - 1 Allocating Major Number dynamically
	
	mydisk_major = register_blkdev(mydisk_major, "mydisk");		//allocates dynamically
	
	if (mydisk_major <= 0) 
	{
		printk(KERN_ALERT "Unable to get major number\n");
		return -EBUSY;
	}
	
	// Step - 2 Spin lock - Allocation of request queue 
	
	spin_lock_init(&mydisk_dev.lock);		// Told later - Prevents kernel from queuing other requests for your device
	
	mydisk_dev.queue = blk_init_queue(mydisk_request, &mydisk_dev.lock);		

	// Req fun associated with req queue
	// Request queue that performs block read and write operations
	// Lock provided by driver rather than the general parts of the kernel because, often, the request queue and other
	// driver data structures fall within the same critical section
	
	
	// Check the return value because any function that performs allocation of memory can fail

	if (mydisk_dev.queue == NULL)
        {
    	    printk(KERN_ERR "blk_init_queue failure\n");	
    	    return -ENOMEM;				
    	}

	
	// Step -3 Allocate initialize and install corresponding gendisk structure
	
	mydisk_dev.gd = alloc_disk(MYDISK_MINORS);		// MYDISK_MINORS = No. of partitions	

	if (!mydisk_dev.gd) 
	{
		printk (KERN_NOTICE "alloc_disk failure\n");
		//goto out_vfree;
	}
	
	mydisk_dev.gd->major = mydisk_major;			// Major no.
	
	mydisk_dev.gd->first_minor = 0;			// First minor no.
    
	mydisk_dev.gd->fops = &mydisk_fops;			

	mydisk_dev.gd->queue = mydisk_dev.queue;

	mydisk_dev.gd->private_data = &mydisk_dev;

	sprintf(mydisk_dev.gd->disk_name,"mydisk");

	set_capacity(mydisk_dev.gd, mydisk_dev.size);

	add_disk(mydisk_dev.gd);

	printk(KERN_INFO "Block Drive Initialised");

	
	return 0;
	
	
}


static void __exit mydisk_exit(void) 
{
	del_gendisk(mydisk_dev.gd);
	put_disk(mydisk_dev.gd);
	blk_cleanup_queue(mydisk_dev.queue);
	vfree(dev_data); 	
	unregister_blkdev(mydisk_major,"mydisk");
}	

module_init(mydisk_init);
module_exit(mydisk_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("IRFAN");
