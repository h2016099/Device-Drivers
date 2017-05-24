#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/jiffies.h>		
#include <linux/ioctl.h>

#define SUCCESS 0
#define MAJOR_NUM 125
#define BUF_LEN 80

static int Device_Open = 0;

int j=0;

unsigned long min,max;

unsigned long rand;


char *Message_Ptr;

char Message[BUF_LEN];

int jiffy;

#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, int)

#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)


 
static dev_t first; // variable for device number
static struct cdev c_dev; // variable for the character device structure
static struct class *cls; // variable for the device class


/*****************************************************************************
STEP 4 as discussed in the lecture, 
my_close(), my_open(), my_read(), my_write() functions are defined here
these functions will be called for close, open, read and write system calls respectively. 
*****************************************************************************/

static int trng_open(struct inode *pinode, struct file *pfile)
{
   
    printk(KERN_INFO "Mychar : open()\n");
    /*
     * We don't want to talk to two processes at the same time
     */
    if (Device_Open)
        return -EBUSY;

    Device_Open++;

    
    try_module_get(THIS_MODULE);
    return SUCCESS;
}	

static ssize_t trng_read(struct file *pfile , char __user *buffer, size_t length, loff_t *offset)
{
	   /*
     * Number of bytes actually written to the buffer
     */
    int bytes_read = 0;	
	

    printk(KERN_INFO "Mychar : read()\n");	
      

    /*
     * If we're at the end of the message, return 0
     * (which signifies end of file)
     */
    if (*Message_Ptr == 0)
        return 0;

    /*
     * Actually put the data into the buffer
     */
    while (length && *Message_Ptr) {

    /*
     * Because the buffer is in the user data segment,
     * not the kernel data segment, assignment wouldn't
     * work. Instead, we have to use put_user which
     * copies data from the kernel data segment to the
     * user data segment.
     */
     put_user(*(Message_Ptr++), buffer++);
     length--;
     bytes_read++;
	}
	printk(KERN_ALERT"No. of bytes %d",bytes_read);
	return bytes_read;

}

static ssize_t trng_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{

	printk(KERN_INFO "Mychar : write()\n");
	
	if(j==0)					// Min. no reading
    	{
		j = j+1;

		min = (unsigned long)buffer;	
	
		printk(KERN_INFO "%lu",min);
		
		//jiffy1 = jiffies;	
	}

	else						// Max. No. reading
	{
		j = 0;
		
		max = (unsigned long)buffer;

		printk(KERN_INFO "%lu",max);

		jiffy = jiffies;			
	
		rand = (jiffy) % max ;

		
	
		while(rand<min || rand>max )		// To get the random number in to the range
		{
		
			if(rand < min)
			{
				rand = rand + min;

			}
			else if(rand > max)
			{
				rand = rand % max;
			}
		
		}		
		
		printk(KERN_INFO "Random no. is %lu",rand);

				

		sprintf(Message,"%lu",rand);
		Message_Ptr = Message; 
	}

    return 0;
}	


int trng_close(struct inode *pinode, struct file *pfile)
{
	printk(KERN_INFO "Mychar : close()\n");
	    /*
	     * We're now ready for our next caller
	     */
	    Device_Open--;
	    module_put(THIS_MODULE);
	    return SUCCESS;
}
	
long device_ioctl(struct file *pfile,             /* ditto */
                  unsigned int ioctl_num,        /* number and param for ioctl */
                  unsigned long ioctl_param)
{
    int num;
 
    int i=0;
  
     /* Switch according to the ioctl called*/

    switch (ioctl_num) {
    case IOCTL_SET_MSG:
        /*
         * Receive a pointer to a message (in user space) and set that
         * to be the device's message.  Get the parameter given to
         * ioctl by the process.
         */
		
		num=ioctl_param;
		
		printk(KERN_ALERT "Received Number is %d \n",num);     

		

         	trng_write(pfile, (char *)ioctl_param, 99, 0);	
			
		break;

	
    case IOCTL_GET_MSG:
        /*
         * Give the current message to the calling process -
         * the parameter we got is a pointer, fill it.
         */
              i = trng_read(pfile, (char *)ioctl_param, 99, 0);

        /*
         * Put a zero at the end of the buffer, so it will be
         * properly terminated
         */
        put_user('\0', (char *)ioctl_param + i);
        break;
    }

    return SUCCESS;
}
//###########################################################################################


struct file_operations trng_dev_file_operations =
{
  .owner 	= THIS_MODULE,
  .open 	= trng_open,
  .release 	= trng_close,
  .read 	= trng_read,
  .write 	= trng_write,
  .unlocked_ioctl = device_ioctl
};
 
//########## INITIALIZATION FUNCTION ##################
// STEP 1,2 & 3 are to be executed in this function ### 
static int __init mychar_init(void) 
{
	printk(KERN_INFO "Namaste: mychar driver registered");
	
	// STEP 1 : reserve <major, minor>
	if (alloc_chrdev_region(&first, 0, 1, "IRFAN") < 0)
	{
		return -1;
	}
	
	// STEP 2 : dynamically create device node in /dev directory
    if ((cls = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		unregister_chrdev_region(first, 1);
		return -1;
	}
    if (device_create(cls, NULL, first, NULL, "trng_dev") == NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	
	// STEP 3 : Link fops and cdev to device node
    cdev_init(&c_dev, &trng_dev_file_operations);
    if (cdev_add(&c_dev, first, 1) == -1)
	{
		device_destroy(cls, first);
		class_destroy(cls);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	return 0;
}
 
static void __exit mychar_exit(void) 
{
	cdev_del(&c_dev);
	device_destroy(cls, first);
	class_destroy(cls);
	unregister_chrdev_region(first, 1);
	printk(KERN_ALERT "Bye: mychar driver unregistered\n\n");
}
 
module_init(mychar_init);
module_exit(mychar_exit);
MODULE_LICENSE("GPL");

