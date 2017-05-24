#include <linux/module.h>							// Core header for loading LKM's
#include <linux/init.h>								// Macros to markup function __init
#include <linux/kernel.h>							// Contains kernel types
#include <linux/gpio.h>								// GPIO Access
#include <linux/interrupt.h>							// For IRQ code
#include <asm/uaccess.h>							// copy_to_user function
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "Light_sensor"

#define I2C_ADDR 0x13								// i2c_slave_address
#define PARAM_REG 0x84								// ambient parameter register
#define PARAM_REG_VALUE 0x9D				// For continous conversion,2 samples/sec, offset compensation, 32 conversions
#define COMMAND_REG 0x80				// Command Reg
#define MEASURE_AMBIENT 0x05				// For measuring ambient light

#define ADAP_NUM 1
#define MAJOR_NO 242

static dev_t first;
struct class *cl;
static struct cdev c_dev;
static struct i2c_client *client;	// To give the I2C bus to a specific client
struct i2c_adapter *adap;			// Adapter for I2C
static u8 lux[2];					// Variable to store the result
static u16 result;			

static u8 i2c_read(struct i2c_client *client,u8 reg)
{

	int ret;
	ret = i2c_smbus_read_byte_data(client,reg);		// read from smbus, from clien and register
	if (ret < 0)									// Returns negative no. on error else data
		dev_err(&client->dev,
			"can not read register, returned %d\n", ret);

	return (u8)ret;

}

static int i2c_write(struct i2c_client *client, u8 reg, u8 data)
{

	int ret;						// write to client via smbus
	ret = i2c_smbus_write_byte_data(client, reg, data);	// Returns negative no. on error else 0
	if (ret < 0)
		dev_err(&client->dev,"can not write register, returned %d\n", ret);

	return ret;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off) {	

	lux[1] = i2c_read(client,0x85);			// Higher Byte result
	lux[0] = i2c_read(client,0x86);			// Lower Byte result
	result = lux[1]*256 + lux[0];
	printk(KERN_INFO "Driver read() %d\n",result);
	
		if (copy_to_user(buf, &result,3) != 0)		// Copy the result into userspace
		{
			printk(KERN_ALERT "Data not copied\n");
			return -EFAULT;
		}
		else
		{
			return 3;								// Bytes_read
		}
	
}


static int my_open(struct inode *i, struct file *f)
{
	  printk(KERN_INFO "Driver: open()\n");
	    return 0;
}
static int my_close(struct inode *i, struct file *f)
{
	  printk(KERN_INFO "Driver: close()\n");
	    return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf,
		   size_t len, loff_t *off)
{
	  printk(KERN_INFO "Driver: write()\n");
	    return len;
}

static struct file_operations device_fops = {.read = my_read,
						.owner = THIS_MODULE,
						.open = my_open,
						.release = my_close,
						.write = my_write};

static int __init ambient__init(void)
{
		u8 readvalue;
		u8 temp;
	
		// Reserve Major and Minor Num
		
		if (alloc_chrdev_region(&first, 0, 1, DEVICE_NAME) < 0) 
		{
        		printk(KERN_DEBUG "Can't register device\n");
       			return -1;
        	}
		
		// Dynamically create device node in /dev directory
		if (IS_ERR(cl = class_create(THIS_MODULE, "chardrv")))
		{
			unregister_chrdev_region(first, 1);
		}

		if (IS_ERR(device_create(cl, NULL, first, NULL,DEVICE_NAME))) 
		{
			class_destroy(cl);
			unregister_chrdev_region(first,1);
		}

		// Link fops and cdev to device node
		cdev_init(&c_dev,&device_fops);

		if (cdev_add(&c_dev, first,1) < 0)
		{
			device_destroy(cl, first);
			class_destroy(cl);
			unregister_chrdev_region(first,1);
			return -1;
		}

		printk(KERN_INFO "Light Sensor Driver Initialised \n");

		adap = i2c_get_adapter(ADAP_NUM);		// To get the pointer to adapter structure

		if (!(client = i2c_new_dummy(adap,I2C_ADDR)))	// To handle the client the adapter bus
		{
			printk(KERN_INFO "Couldn't acquire i2c slave");
			unregister_chrdev_region(first, 1);
			device_destroy(cl, first);
			class_destroy(cl);
			return -1;
		}

		readvalue = i2c_read(client,0x81);	// Detect the device

		if(readvalue == 0x21)			//Register data value of current version
		{
			printk(KERN_INFO "Device detected, value is %d",readvalue);
		}

		temp = i2c_write(client, COMMAND_REG,MEASURE_AMBIENT);		// Passing the command value into the register

	
		lux[1] = i2c_read(client,0x85);		// Reading the higher byte value

		lux[0] = i2c_read(client,0x86);		// Reading the lower byte value

		result = lux[1]*256 + lux[0];		// Calculating the Light Intensity value

		printk(KERN_ALERT "Luminance value is %d",result);

		return 0;

}

static void __exit ambient_exit(void)
{

		printk(KERN_INFO "Light Sensor Driver Removed \n");
		i2c_unregister_device(client);	// Unregister the client
        	cdev_del(&c_dev);		// unregister the char driver
	 	device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);

}


module_init(ambient__init);
module_exit(ambient_exit);

MODULE_LICENSE("GPL");   
MODULE_AUTHOR("Irfan");  
MODULE_DESCRIPTION("Ambient Light Sensor LKM for the RPi.");

