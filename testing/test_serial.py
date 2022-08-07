import serial
import time
import eyeD3

# TEST TIMING FOR CONTROLLING THE LEDS FROM PYTHON

arduino = serial.Serial(port="COM5", baudrate=2000000, timeout=1,)


def showPixeles():
    arduino.write((255,0))

def setPixel(pixelIndex:int, R:int, G:int, B:int):
    arduino.write((pixelIndex>>8, pixelIndex%256, R, G, B))


print("hey, starting!")
print(time.clock())
time.sleep(1)

t1 = time.clock()
for i in range (0,500):
    setPixel(i, i%256, i%256, i%256)
t2 = time.clock()
showPixeles()
t3 = time.clock()

print("timing for changing 500 pixels: ", t2-t1)
print("timing for showing pixels: ", t3-t2)
print("end.")