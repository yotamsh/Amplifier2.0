#include <FastLED.h>
#define NUM_LEDS 300
#define LED_DATA_PIN 12
#define NUM_BUTTONS 10

CRGB leds[NUM_LEDS];
int ind = 0;


void setup() {
  // put your setup code here, to run once:
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical  
  FastLED.setBrightness(20);
  Serial.begin(9600);
  
}

struct Baloon{
  byte center;
  byte movingIndex;
  byte start;
  byte endd;
  byte hue;
  
  Baloon(int start, int endd, uint8_t hue){
    center = random8(start+5, endd-5);
    movingIndex = 0;
    this->start = start;
    this->endd = endd; 
    this->hue = hue;
  }

  bool promote(CRGB* arr){
    byte finished = 0;
    int index = center+movingIndex;
    if(index < endd) arr[index] = CHSV(hue, 255,255);
    else finished++;
    index = center-movingIndex;
    if(start <= index) arr[index] = CHSV(hue, 255, 255);
    else finished++;
    movingIndex++;
    hue+=2;
    return (finished == 2);
  }
};
//typedef struct baloon ButtonsState;

Baloon currentBaloon = Baloon(0,30,random8(255));
byte tetrisHue = 0;
byte rainbowHue = 0;

void loop() { 
  EVERY_N_MILLISECONDS(20){
    if(currentBaloon.promote(leds)){
      currentBaloon = Baloon(0,30,beat8(47));
    }

    EVERY_N_MILLISECONDS(200){
      uint8_t tetris = random8(3,11);
      uint8_t pos = random8(35,60-tetris+1);
      fadeToBlackBy(&leds[35],30,50);
      fill_solid(&leds[pos], tetris, CHSV(tetrisHue,random8(100,256),255));
      tetrisHue += 37;
    }

    fill_rainbow(&leds[70], 30, beatsin8(11)+beatsin8(30)+beatsin8(41), 255/30);

    EVERY_N_MILLISECONDS(200){
      if(beatsin8(231)>127){
        uint8_t pos = random8(105,135+1);
        leds[pos] = CRGB(255,random8(200,255),random8(100,255));
      }
    }
    fadeToBlackBy(&leds[105],30,5);

    byte edge = beatsin8(11,0,10)+beatsin8(30, 0, 10)+beatsin8(47,0,9);
    fill_solid(&leds[140], edge, CHSV(beatsin8(20,0,30),255,255));
    fill_solid(&leds[140+edge], 30-edge, CHSV(beatsin8(18,100,150),255,255));
    
    
    FastLED.show();
  }
  
}
