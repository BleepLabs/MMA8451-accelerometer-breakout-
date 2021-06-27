# MMA8451-accelerometer-breakout  
  
![](https://raw.githubusercontent.com/BleepLabs/MMA8451-accelerometer-breakout-/main/MMA8451_PCB.png)   
I made this breakout a long time ago for a product taht didn't happend so thy've jsut been sitting around. Later, [Adafruit](https://www.adafruit.com/product/2019) came out with an PCB and library much better than my own but this little breakout still works well.  
  
The only thing to change when using the adafruit library is the i2c address. Pin 4 is the address select and it's tied to ground. Use `accelo.begin(0x1C)` to start the device.  
  
See the incuded example for more info.    
