/**************READ ME *****************/

1) type 'make all' command it wil build main.ko

2) Then insert the module using 'sudo insmod main.ko'

3) Then for writing and reading from the device, change the permissions of the device driver file using 'sudo chmod 777 /dev/trng_dev'

4) Then compile your user space code using gcc compiler 'gcc -g user.c -o out', this will generate an executable file 'out'

5) Run the executable file using './out'

6) Enter the Lower no. and the Upper no. of the range.

7) Output will be the random number in the given range.

Description:-
		This is the kernel module to generate true random number. It make use of jiffies, to get the number of timer ticks from the time of system booting and it depends on the frequency of system timer.
	We use IOCTL system call for communication between kernel and the user space.

