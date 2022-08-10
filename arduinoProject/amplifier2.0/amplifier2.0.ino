// ---
// --- AMPLIFIER 2.0 CODE | Yotam Shkedi (C)
// --- .this is the buttons and leds module. 
// --- .music module runs in python through serial com.
// --- 

#include <FastLED.h>

#define NUM_LEDS 300
#define LED_DATA_PIN 12
#define NUM_BUTTONS 10
#define CLICKED 1
#define UNCLICKED 0
#define SEVEN_SPEED 1000
#define LED_LEFT_EDGE_INDEX 0
#define LED_RIGHT_EDGE_INDEX 299
#define LED_MIDDLE_INDEX 150
#define BUTTON_NOT_EXIST (NUM_BUTTONS+1)


#define IDLE_MODE 0b1
#define AMPLIFYING_MODE 0b10
#define PARTY_MODE 0b100
#define CODE_INPUT_MODE 0b1000

// pins of button number {0, 1, ... , 9}
byte buttonPins[NUM_BUTTONS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
boolean testing = true;

struct buttons_state{
  byte prev[NUM_BUTTONS] = {0};
  byte current[NUM_BUTTONS] = {0};
  bool waitRelease[NUM_BUTTONS] = {false};
  byte total = 0;
  byte prevTotal= 0;

  void updateState(){
    prevTotal = total;
    total = 0;
    for (byte i=0 ; i < NUM_BUTTONS ; i++){
      prev[i] = current[i];
      current[i] = (digitalRead(buttonPins[i]) == LOW) ? CLICKED : UNCLICKED;
      if(current[i]==0) waitRelease[i]=false;
      if(waitRelease[i]) current[i] = UNCLICKED;
      total += current[i];
    }
    if(testing && current[1]){
      total = 10;
      for (byte i=0 ; i < NUM_BUTTONS ; i++){ current[i]=CLICKED; }   
    }
  }
  
  void setWaitReleaseAll(){
    for(byte i=0; i<NUM_BUTTONS; i++) 
      if(current[i]) waitRelease[i] = true;    
  }
  void setWaitRelease(byte button){
    if(current[button]) waitRelease[button] = true;    
  }
  
};
typedef struct buttons_state ButtonsState;


ButtonsState buttonsState;
CRGB leds[NUM_LEDS];

byte mode;
byte sevenCounter;
unsigned long sevenTimestamp;

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical  
  FastLED.setBrightness(20);

  for (byte i=0 ; i < NUM_BUTTONS ; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
    //digitalWrite(buttonPins[i], HIGH);
    buttonsState.waitRelease[i] = true;
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  mode = IDLE_MODE;
  sevenCounter = 0;
  sevenTimestamp = 0;
  
  //pinMode(12, OUTPUT);
}


// -- MAIN LOOP -- //
void loop() {
  switch (mode){
    case IDLE_MODE:
      runIdleMode();
      break;
    case AMPLIFYING_MODE:
      runAmplifyingMode();
      break;
    case PARTY_MODE:
      runPartyMode();
      break;
    case CODE_INPUT_MODE:
      runCodeInputMode();
      break;
  }
}

// modes //

void runIdleMode(){
  // init mode
  sendNumClicked(0);
  buttonsState.setWaitReleaseAll();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  unsigned long idleModeStartTime = millis();
  int hueIndex = 0;
  int ledIndex = random16(NUM_LEDS);
  bool reverseDirection = random8(2);
  
  // run mode
  while(mode == IDLE_MODE){
    unsigned long currentTime = millis();
    buttonsState.updateState();
    if(buttonsState.total > 0){
      updateSevenCounter(currentTime);
      if(sevenCounter == 3){
        mode = CODE_INPUT_MODE;
      }
      else {
        mode = AMPLIFYING_MODE;
      }
      break;
    }
    // continue running
    // lighting
    if (currentTime - idleModeStartTime > 3000){
      EVERY_N_MILLISECONDS(20){

        leds[ledIndex].setHSV(hueIndex, 255, 255);
        if (reverseDirection) ledIndex--; else ledIndex++;
        if (ledIndex==NUM_LEDS-1 || ledIndex==0) reverseDirection = !reverseDirection;
        hueIndex += 3;
        fadeToBlackBy(leds, NUM_LEDS, 40);
        FastLED.show();
      }
    }
  }
  // end mode
}

void runAmplifyingMode(){
  if(buttonsState.total != NUM_BUTTONS){
    sendNumClicked(buttonsState.total);
  }
  //byte buttonAnimationIndex[10] = {0};
  int activeAnimationIndex = 0;
  bool reverse = false;
  bool activeAnimationHue = 0;
  
  while(mode == AMPLIFYING_MODE){
    buttonsState.updateState();
    if(buttonsState.total == 0){
      mode = IDLE_MODE;
      break;
    }
    if(buttonsState.total == NUM_BUTTONS){
      sendNumClicked(10);
      mode = PARTY_MODE;
      break;
    }
    if(Serial.available()){
      int msg = Serial.read();
      mode = IDLE_MODE;
      break;
    }

    // continue running
    if(buttonsState.total != buttonsState.prevTotal){
      sendNumClicked(buttonsState.total);
      unsigned long currentTime = millis();
      updateSevenCounter(currentTime);
    }
    // lighting
    EVERY_N_MILLISECONDS(20){
      for (byte i=0; i<NUM_BUTTONS; i++){
        if (buttonsState.current[i] && !buttonsState.prev[i])
          activeAnimationIndex = i*(NUM_LEDS/NUM_BUTTONS) + (reverse ? (NUM_LEDS/NUM_BUTTONS)-1 : 0);
      }
      byte button = activeAnimationIndex/(NUM_LEDS/NUM_BUTTONS);
      if(buttonsState.current[button]
          && (activeAnimationIndex != button*NUM_LEDS/NUM_BUTTONS + (reverse ? 0 : NUM_LEDS/NUM_BUTTONS-1))){
        activeAnimationIndex = reverse ? activeAnimationIndex-1 : activeAnimationIndex+1;
      }
      else{
        byte nextActiveButton = findNextActiveButtonAndDirection(button, reverse);
        if (nextActiveButton != BUTTON_NOT_EXIST){
          activeAnimationIndex = nextActiveButton*NUM_LEDS/NUM_BUTTONS + (reverse ? NUM_LEDS/NUM_BUTTONS-1 : 0);
        }
      }
      uint8_t hue = beat8(30);
      leds[activeAnimationIndex] = CHSV(hue,255,255);

      for(byte i=0; i<NUM_BUTTONS; i++){
        if (buttonsState.current[i] == UNCLICKED){
          fill_solid(&leds[i*30], 30, CRGB::Black);
          if (beat8(20)>240) leds[i*30+15] = CRGB::Orange;
        }
      }
      FastLED.show();
    }
  }
}

void runPartyMode(){
  buttonsState.setWaitReleaseAll();
  bool ledOn = false;
  byte leftMuteIndex = 0;
  byte rightMuteIndex = 0;
  byte muteLevel = 0;
  unsigned long partyModeStartTime = millis();
  
  while(mode == PARTY_MODE){
    if(Serial.available()){
      int msg = Serial.read();
      mode = IDLE_MODE;
      break;
    }
    // continue running
    unsigned long currentTime = millis();
    if(currentTime - partyModeStartTime > 5000){
      buttonsState.updateState();
      if(buttonsState.current[0]==CLICKED){
        EVERY_N_MILLISECONDS(30) { 
          if(leftMuteIndex<LED_MIDDLE_INDEX) leftMuteIndex++;
        }
      }
      else resetMuteIndex(leftMuteIndex, muteLevel);
      
      if(buttonsState.current[9]==CLICKED){
        EVERY_N_MILLISECONDS(30) {
          if(rightMuteIndex<LED_MIDDLE_INDEX) rightMuteIndex++;
        }
      } 
      else resetMuteIndex(rightMuteIndex, muteLevel);
      
      bool othersFail = false;
      for (byte i=1; i < NUM_BUTTONS-1; i++){
        if(buttonsState.current[i]==CLICKED) othersFail = true;
      }
      if (othersFail){
        resetMuteIndex(rightMuteIndex, muteLevel);
        resetMuteIndex(leftMuteIndex, muteLevel);
        buttonsState.setWaitRelease(0);
        buttonsState.setWaitRelease(9);
      }
      if (leftMuteIndex == LED_MIDDLE_INDEX 
          && rightMuteIndex == LED_MIDDLE_INDEX){
        EVERY_N_MILLISECONDS(1000) {
          muteLevel++;
          sendValue(muteLevel);
        }
      }
      if (muteLevel == NUM_BUTTONS){
        mode = IDLE_MODE;
        break;        
      }
    }
    
    // lighting    
    EVERY_N_MILLISECONDS(20){
      if (muteLevel == 0){
        fillLeds(LED_LEFT_EDGE_INDEX+leftMuteIndex, LED_RIGHT_EDGE_INDEX-rightMuteIndex, CRGB::Black);
        for(byte i=0; i<leftMuteIndex; i++){
          leds[LED_LEFT_EDGE_INDEX+i]+= CRGB(20, 0, 0);
          leds[LED_LEFT_EDGE_INDEX+i]-= CRGB(0, 50, 50);
        }
        for(byte i=0; i<rightMuteIndex; i++){
          leds[LED_RIGHT_EDGE_INDEX-i]+= CRGB(20, 0, 0);
          leds[LED_RIGHT_EDGE_INDEX-i]-= CRGB(0, 50, 50);
        }
      }
      else {
        fill_solid(&leds[NUM_LEDS/2-muteLevel*NUM_LEDS/(2*NUM_BUTTONS)], muteLevel*NUM_LEDS/NUM_BUTTONS, CHSV(160, 255,100));
      }
      FastLED.show();
    }
  }
  // party mode end
}

void runCodeInputMode(){  
  sendValue(77);
  enterCodeInputMode();
  unsigned long codeInputModeTimestamp = millis();
  byte codeSize = 0;
  byte code[5] = {0};

  while(mode == CODE_INPUT_MODE){
    delay(100);
    buttonsState.updateState();
    bool failed = false;
    updateCodeInput(code, codeSize, codeInputModeTimestamp, failed);
    if(failed){
      sendValue(88);
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      FastLED.show();
      delay(3000); // failed sound
      mode = IDLE_MODE;
      break;
    }
    else if (codeSize == 5) {
      sendCode(code);
      while(!Serial.available()){}
      int msg = Serial.read();
      if (msg == 1) { // valid code
        mode = PARTY_MODE;
        delay(3100); // party mode intro
        break;        
      }
      else{ // invalid code
        mode = IDLE_MODE;
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.show();
        delay(3000); // failed sound
        break;  
      }
    }
    // continue running
    // lighting
  } 
}



// methods //

void resetMuteIndex(byte &muteIndex, byte &muteLevel){
  muteIndex = 30;
  if (muteLevel > 0){
    muteLevel = 0;
    sendValue(muteLevel);
  }
}

void updateCodeInput(
    byte code[], 
    byte &codeSize, 
    unsigned long codeInputModeTimestamp, bool &failed){
  unsigned long currentTime = millis();
  if(currentTime - codeInputModeTimestamp > 38661){
    failed = true;
    return;
  }
  for (byte i=0; i < codeSize ; i++){
    if(buttonsState.prev[code[i]] && buttonsState.current[code[i]]==UNCLICKED){
        failed = true;
        return;      
    }
  }
  boolean change = false;
  for (byte i=0; i<NUM_BUTTONS ; i++){
    if(buttonsState.prev[i]==UNCLICKED && buttonsState.current[i]){
      sendValue(i+65);
      if(change){
        failed = true;
        return;
      }
      change = true;
      code[codeSize] = i;
      codeSize++;
      fill_solid(&leds[i*30], 29, CRGB::Green);
      FastLED.show();
    }
  }
}

void updateSevenCounter(unsigned long currentTime){
  if(currentTime - sevenTimestamp > SEVEN_SPEED){
    sevenCounter = 0;
  }
  if(buttonsState.current[7] && buttonsState.prev[7]==UNCLICKED){
    sevenCounter++;
    sevenTimestamp = currentTime;
  }
  for (byte i=0; i<NUM_BUTTONS ; i++){
    if(i != 7 && (buttonsState.current[i] || buttonsState.waitRelease[i])){
      sevenCounter = 0;
    }    
  }
}

void sendNumClicked(byte numOfClicked)
{
  Serial.write(numOfClicked);
  Serial.flush();
  delay(10);
}

void sendValue(byte value)
{
  Serial.write(value);
  Serial.flush();
}

void sendCode(byte code[]){
  Serial.write(99);
  for(byte i=0; i<5; i++){
    Serial.write(code[i]+48);
  }
  Serial.flush();
}

void enterCodeInputMode(){
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  for (byte greenIndex=0; greenIndex<LED_MIDDLE_INDEX; greenIndex=greenIndex+2){
    fill_solid(&leds[LED_MIDDLE_INDEX], greenIndex, CRGB::Green);
    fill_solid(&leds[LED_MIDDLE_INDEX-greenIndex], greenIndex, CRGB::Green);
    FastLED.show();
  }
  fill_solid(leds, NUM_LEDS, CHSV(HUE_PURPLE,255,50));
  for (byte i=0; i<NUM_BUTTONS; i++){
    fill_solid(&leds[i*30+10], i, CRGB::White);
  }
  FastLED.show();
}

byte findNextActiveButtonAndDirection(byte button, bool &reversed){
  button = reversed ? button-1 : button+1;
  while(0 <= button && button < NUM_BUTTONS){
    if(buttonsState.current[button]) return button;
    button = reversed ? button-1 : button+1;
  }
  reversed = !reversed;
  button = reversed ? button-1 : button+1;
  while(0 <= button && button < NUM_BUTTONS){
    if(buttonsState.current[button]) return button;
    button = reversed ? button-1 : button+1;
  }
  return BUTTON_NOT_EXIST ;
}

void fillLeds(int from, int to, CRGB value){
  for (int i=from; i < to; i++){
    leds[i] = value;
  }
}
//
//void decreaseByte(byte &ref, byte dif){
//  ref = (ref<dif) ? 0 : ref-dif;
//}
//
//void increaseByte(byte &ref, byte dif){
//  ref = (ref>255-dif) ? 255 : ref+dif;
//}
//void blinkLed(byte times) {
//  for (byte i=0; i<times ; i++){
//    digitalWrite(12, HIGH);
//    delay(200);
//    digitalWrite(12, LOW);
//    delay(200);
//  }
//}
