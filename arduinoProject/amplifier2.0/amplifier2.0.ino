// ---
// --- AMPLIFIER 2.0 CODE | Yotam Shkedi (C)
// --- .this is the buttons and leds module. 
// --- .music module runs in python through serial com.
// --- 

#include <FastLED.h>

#define NUM_LEDS 300
#define LED_DATA_PIN 13
#define NUM_BUTTONS 10
#define SEVEN_SPEED 1000
#define LED_LEFT_EDGE_INDEX 0
#define LED_RIGHT_EDGE_INDEX 300
#define LED_MIDDLE_INDEX 150

#define IDLE_MODE 0b1
#define AMPLIFYING_MODE 0b10
#define PARTY_MODE 0b100
#define CODE_INPUT_MODE 0b1000

// pins of button number {0, 1, ... , 9}
byte buttonPins[NUM_BUTTONS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
boolean testing = false;

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
      current[i] = (digitalRead(buttonPins[i]) == LOW) ? 1 : 0;
      if(current[i]==0) waitRelease[i]=false;
      if(waitRelease[i]) current[i] = 0;
      total += current[i];
    }
    if(testing && current[1]){
      total = 10;
      for (byte i=0 ; i < NUM_BUTTONS ; i++){ current[i]=1; }   
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
  FastLED.addLeds<WS2812B, LED_DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical  

  for (byte i=0 ; i < NUM_BUTTONS ; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
    //digitalWrite(buttonPins[i], HIGH);
    buttonsState.waitRelease[i] = true;
  }
  mode = IDLE_MODE;
  sevenCounter = 0;
  sevenTimestamp = 0;
  
  pinMode(12, OUTPUT);
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

void runIdleMode(){
  // init mode
  sendNumClicked(0);
  buttonsState.setWaitReleaseAll();
  
  // run mode
  while(mode == IDLE_MODE){
    buttonsState.updateState();
    if(buttonsState.total > 0){
      unsigned long currentTime = millis();
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
  }
  // end mode
}

void runAmplifyingMode(){
  if(buttonsState.total != 10){
    sendNumClicked(buttonsState.total);
  }
  while(mode == AMPLIFYING_MODE){
    buttonsState.updateState();
    if(buttonsState.total == 0){
      mode = IDLE_MODE;
      break;
    }
    if(buttonsState.total == 10){
      sendNumClicked(10);
      mode = PARTY_MODE;
      break;
    }
    // continue running
    if(buttonsState.total != buttonsState.prevTotal){
      sendNumClicked(buttonsState.total);
      unsigned long currentTime = millis();
      updateSevenCounter(currentTime);
    }
    // lighting
  }
}

void runPartyMode(){
  buttonsState.setWaitReleaseAll();
  bool ledOn = false;
  byte leftMuteIndex = 0;
//  byte leftMuteAnimationIndex = 0;
  byte rightMuteIndex = 0;
//  byte rightMuteAnimationIndex = 0;
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
      if(buttonsState.current[0]==1){
        EVERY_N_MILLISECONDS(30) { 
          if(leftMuteIndex<LED_MIDDLE_INDEX) leftMuteIndex++;
        }
      }
      else resetMuteIndex(leftMuteIndex, muteLevel);
      
      if(buttonsState.current[9]==1){
        EVERY_N_MILLISECONDS(30) {
          if(rightMuteIndex<LED_MIDDLE_INDEX) rightMuteIndex++;
        }
      } 
      else resetMuteIndex(rightMuteIndex, muteLevel);
      
      bool othersFail = false;
      for (byte i=1; i < NUM_BUTTONS-1; i++){
        if(buttonsState.current[i]==1) othersFail = true;
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
    EVERY_N_MILLISECONDS(500){
      ledOn = !ledOn;
      digitalWrite(12, ledOn);
    }
    EVERY_N_MILLISECONDS(200){
      for(byte i=0; i<leftMuteIndex; i++){
        leds[LED_LEFT_EDGE_INDEX+i] = CRGB::Red;
      }
      for(byte i=0; i<rightMuteIndex; i++){
        leds[LED_RIGHT_EDGE_INDEX-i] = CRGB::Red;
      }
    }
  }
  // party mode end
  ledOn = false;
  digitalWrite(12, false);
}

void runCodeInputMode(){
  unsigned long codeInputModeTimestamp = millis();
  Serial.write(77);
  Serial.flush();
  byte codeSize = 0;
  byte code[5] = {0};

  while(mode == CODE_INPUT_MODE){
    delay(100);
    buttonsState.updateState();
    bool failed = false;
    updateCodeInput(code, codeSize, codeInputModeTimestamp, failed);
    if(failed){
      sendValue(88);
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
        delay(5000); // failed sound
        break;  
      }
    }
    // continue running
    // lighting
  } 
}

// methods
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
    if(buttonsState.prev[code[i]] && buttonsState.current[code[i]]==0){
        failed = true;
        return;      
    }
  }
  boolean change = false;
  for (byte i=0; i<NUM_BUTTONS ; i++){
    if(buttonsState.prev[i]==0 && buttonsState.current[i]){
      sendValue(i+65);
      if(change){
        failed = true;
        return;
      }
      change = true;
      code[codeSize] = i;
      codeSize++;
    }
  }
}

void updateSevenCounter(unsigned long currentTime){
  if(currentTime - sevenTimestamp > SEVEN_SPEED){
    sevenCounter = 0;
  }
  if(buttonsState.current[7] && buttonsState.prev[7]==0){
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

void blinkLed(byte times) {
  for (byte i=0; i<times ; i++){
    digitalWrite(12, HIGH);
    delay(200);
    digitalWrite(12, LOW);
    delay(200);
  }
}
