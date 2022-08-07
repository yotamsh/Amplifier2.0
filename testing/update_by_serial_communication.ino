#include <FastLED.h>

#define NUM_LEDS 300
#define DATA_PIN 3

CRGB leds[NUM_LEDS];

byte packet_5byte[5];
char buffer[40];


// LISTEN TO SERIAL FOR UPDATING AND SHOWING LEDS

void setup() {
  Serial.begin(2000000);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical  
}

void loop() {  
  while(!Serial.available());
  size_t numChars = Serial.readBytes(packet_5byte, 5);
//  Serial.print("received ");
//  Serial.print(numChars);
//  Serial.print(" chars: ");
//  for (byte i = 0; i<numChars; i++){
//    Serial.print(packet_5byte[i]); Serial.print(" ");
//  }
  
  if(packet_5byte[0] == 255){
    FastLED.show();
//    Serial.print("loop ends. leds Show");
  }
  else{
    int ledIndex = (packet_5byte[0] << 8) + packet_5byte[1];
    CRGB pixel = CRGB(packet_5byte[2], packet_5byte[3], packet_5byte[4]);
    leds[ledIndex] = pixel;

//    sprintf(buffer, "loop ends. pixel index %d was set to R:%d, G:%d, B:%d", ledIndex, pixel.r, pixel.g, pixel.b);
//    Serial.println(buffer);

  }
  //delay(2000);
}
