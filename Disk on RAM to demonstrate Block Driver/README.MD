Description:- This is the kernel module to create a module for block device called "MyDisk",by allocating 512 KB of space in the memory and simulate that as a separate block device and partition it into 3 primary and 3 logical partitions. Block driver can read and write to this virtual disk.

/**************READ ME *****************/

1. type 'make all' command it wil build mydisk.ko

2. Then insert the module using 'sudo insmod mydisk.ko'

3. Then for writing and reading from the device, go into root mode, by giving 'sudo -s' command

4. $ cat > /dev/mydisk1 (for eg.) to write into the device, and write whatever data you want to write. 

5. $ xxd /dev/mydisk1 gives whatever you have written previously into it.

6. $ sudo fdisk -l shows you the partitions made on your device.

 
