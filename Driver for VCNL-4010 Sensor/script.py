# Script to display accelerometer data.
from time import sleep

fd = open('/dev/Light_sensor','r')
while(1):
    data = fd.read(3);
    foo = [ord(i) for i in data]    #ord(i) converts hexadecimal to decimal
    res = foo[1]*256+foo[0]
    print (res)
    sleep(0.5)

    
