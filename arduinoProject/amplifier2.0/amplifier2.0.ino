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
#define FIREWORK_LEN 15
#define CODE_LENGTH 5

#define IDLE_MODE 0b1
#define AMPLIFYING_MODE 0b10
#define PARTY_MODE 0b100
#define CODE_INPUT_MODE 0b1000

// pins of button number {0, 1, ... , 9}
byte buttonPins[NUM_BUTTONS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
boolean testing = true;

// Structs //

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

struct firework {
  int m_startIndex;
  int m_index;
  CRGB m_color;
  
  firework() {
    m_startIndex = -1;
  }
  void initialize(int startIndex, CRGB color, bool reverse){
    m_index = startIndex;
    m_startIndex = startIndex;
    m_color = color;
    //color.red += color.red%2; // now color.red is even
    //color.red += reverse ? 1 : 0; // now color.red is odd <=> reverse direction
    m_color.setParity(reverse ? 1 : 0);
  }
  bool isFinished(){
    return m_startIndex == -1 || (m_index < -FIREWORK_LEN || m_index >= NUM_LEDS+FIREWORK_LEN);
  }
  bool promote(CRGB* leds){
    if (isFinished()) return false;
    for(byte i=0; i<FIREWORK_LEN; i++){
      if(m_color.getParity()){
        if (0 <= m_index+i && m_index+i <= m_startIndex){
          leds[m_index+i] = leds[m_index+i].lerp8(m_color, 255-i*(255/FIREWORK_LEN));
        }
      }
      else{
        if (m_startIndex <= m_index-i && m_index-i < NUM_LEDS){
          leds[m_index-i] = leds[m_index-i].lerp8(m_color, 255-i*(255/FIREWORK_LEN));
        }
      }
    }
    m_index += (m_color.getParity()) ? -1 : 1;
  }
};
typedef struct firework Firework;

// GLOBALS //

ButtonsState buttonsState;
CRGB leds[NUM_LEDS];

byte mode;
byte sevenCounter;
unsigned long sevenTimestamp;

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical  
  FastLED.setBrightness(20);
  FastLED.setCorrection(TypicalSMD5050);

  for (byte i=0 ; i < NUM_BUTTONS ; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
    buttonsState.waitRelease[i] = true;
  }
  mode = IDLE_MODE;
  sevenCounter = 0;
  sevenTimestamp = 0;
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
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

// -- MODES -- //

// IDLE MODE //
void runIdleMode(){
  // init mode
  sendNumClicked(0);
  buttonsState.updateState();
  buttonsState.setWaitReleaseAll();
  //fill_solid(leds, NUM_LEDS, CRGB::Black);
  //FastLED.show();
  unsigned long idleModeStartTime = millis();
  int hueIndex = 0;
  int ledIndex = random16(NUM_LEDS);
  bool reverse = random8(2);
  
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
    if (currentTime - idleModeStartTime > 5000){
      EVERY_N_MILLISECONDS(20){
        updateIdleModeLighting(ledIndex, hueIndex, reverse);
      }
    }
    else{
      EVERY_N_MILLISECONDS(10){
        fadeToBlackBy(leds, NUM_LEDS, 5);
        FastLED.show();
      }
    }
  }
  // exit mode
}

void updateIdleModeLighting(int &ledIndex, int &hueIndex, bool &reverse){
  leds[ledIndex].setHSV(hueIndex, 255, 255);
  if (reverse) ledIndex--; else ledIndex++;
  if (ledIndex==NUM_LEDS-1 || ledIndex==0) reverse = !reverse;
  hueIndex += 3;
  fadeToBlackBy(leds, NUM_LEDS, 40);
  FastLED.show();
}


// AMPLIFYING MODE //
void runAmplifyingMode(){
  int animationIndex = 0;
  bool reverse = false;
  if(buttonsState.total != NUM_BUTTONS){
    sendNumClicked(buttonsState.total);
  }
    
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
      updateAmplifyingModeLighting(animationIndex, reverse);
    }
  }
}

void updateAmplifyingModeLighting(int &animationIndex, bool &reverse){
  // start from a recent-clicked-button
  for (byte i=0; i<NUM_BUTTONS; i++){
    if (buttonsState.current[i] && !buttonsState.prev[i])
      animationIndex = i*(NUM_LEDS/NUM_BUTTONS) + (reverse ? (NUM_LEDS/NUM_BUTTONS)-1 : 0);
  }
  // promote amination index
  byte button = animationIndex/(NUM_LEDS/NUM_BUTTONS);
  if(buttonsState.current[button]
      && (animationIndex != button*NUM_LEDS/NUM_BUTTONS + (reverse ? 0 : NUM_LEDS/NUM_BUTTONS-1))){
    animationIndex = reverse ? animationIndex-1 : animationIndex+1;
  }
  else{
    byte nextActiveButton = findNextActiveButtonAndDirection(button, reverse);
    if (nextActiveButton != BUTTON_NOT_EXIST){
      animationIndex = nextActiveButton*NUM_LEDS/NUM_BUTTONS + (reverse ? NUM_LEDS/NUM_BUTTONS-1 : 0);
    }
  }
  // set leds
  uint8_t hue = beat8(30);
  leds[animationIndex] = CHSV(hue,255,255);

  for(byte i=0; i<NUM_BUTTONS; i++){
    if (buttonsState.current[i] == UNCLICKED){
      fill_solid(&leds[i*30], 30, CRGB::Black);
      if (beat8(20)>240) leds[i*30+15] = CRGB::Orange;
    }
  }
  FastLED.show();
}

// PARTY MODE //
void runPartyMode(){
  byte startAnimation = 0;
  byte leftMuteIndex = 0;
  byte rightMuteIndex = 0;
  byte muteLevel = 0;
  unsigned long partyModeStartTime = millis();
  buttonsState.setWaitReleaseAll();
  Firework fireworks[NUM_BUTTONS];
  
  while(mode == PARTY_MODE){
    if(Serial.available()){
      int msg = Serial.read();
      mode = IDLE_MODE;
      break;
    }
    // continue running
    buttonsState.updateState();
    for(byte i=1; i<NUM_BUTTONS-1; i++){
      if(buttonsState.current[i]==CLICKED && buttonsState.prev[i]==UNCLICKED && fireworks[i].isFinished()){
        fireworks[i].initialize((int)(NUM_LEDS/NUM_BUTTONS*i), (CRGB)CRGB::White, random8(2)==0); 
      }
    }
    unsigned long currentTime = millis();
    if(currentTime - partyModeStartTime > 9000){
      EVERY_N_MILLISECONDS(30){
        if(buttonsState.current[0]==CLICKED){
          if (leftMuteIndex<LED_MIDDLE_INDEX) leftMuteIndex++;
        }
        else resetMuteIndex(leftMuteIndex, muteLevel);
        if(buttonsState.current[9]==CLICKED){
          if(rightMuteIndex<LED_MIDDLE_INDEX) rightMuteIndex++;
        }
        else resetMuteIndex(rightMuteIndex, muteLevel);
      }
      
      bool othersFail = false;
      for (byte i=1; i < NUM_BUTTONS-1; i++){
        if(buttonsState.current[i]==CLICKED) othersFail = true;
      }
      if (othersFail){
        resetMuteIndex(rightMuteIndex, muteLevel);
        resetMuteIndex(leftMuteIndex, muteLevel);
        buttonsState.setWaitRelease(0);
        buttonsState.setWaitRelease(NUM_BUTTONS-1);
      }
      if (leftMuteIndex == LED_MIDDLE_INDEX 
          && rightMuteIndex == LED_MIDDLE_INDEX){
        EVERY_N_MILLISECONDS(1000) {
          muteLevel++;
          sendValue(muteLevel+20);
        }
      }
      if (muteLevel == NUM_BUTTONS){
        mode = IDLE_MODE;
        break;        
      }
    }
    else if (currentTime-partyModeStartTime>8000){
      EVERY_N_MILLISECONDS(30){
        if (leftMuteIndex<30){
          leftMuteIndex++;
          rightMuteIndex++;
        }
      }
    }
    
    // lighting    
    if (startAnimation < NUM_LEDS/2){
      fill_rainbow(&leds[LED_LEFT_EDGE_INDEX+leftMuteIndex], NUM_LEDS-leftMuteIndex-rightMuteIndex, beatsin8(7)+beatsin8(15)+beatsin8(19)+leftMuteIndex, 5);
      fill_solid(leds, NUM_LEDS/2-startAnimation, CRGB::Black);
      fill_solid(&leds[NUM_LEDS/2+startAnimation], NUM_LEDS/2-startAnimation, CRGB::Black);
      FastLED.show();
      startAnimation+=2;
    }
    else{
      EVERY_N_MILLISECONDS(20){
        updatePartyModeLighting(startAnimation, leftMuteIndex, rightMuteIndex, muteLevel, fireworks);
      }
    }
  }
  // exit party mode
}

void updatePartyModeLighting(byte startAnimation, byte leftMuteIndex, byte rightMuteIndex, byte muteLevel, Firework* fireworks){
  
  if (muteLevel == 0){
    fill_rainbow(&leds[LED_LEFT_EDGE_INDEX+leftMuteIndex], NUM_LEDS-leftMuteIndex-rightMuteIndex, beatsin8(11)+beatsin8(30)+beatsin8(41)+leftMuteIndex, 5);
    for(byte i=0; i<leftMuteIndex; i++){
      leds[LED_LEFT_EDGE_INDEX+i]+= CRGB(10, 0, 0);
      leds[LED_LEFT_EDGE_INDEX+i]-= CRGB(0, 10, 10);
    }
    for(byte i=0; i<rightMuteIndex; i++){
      leds[LED_RIGHT_EDGE_INDEX-i]+= CRGB(10, 0, 0);
      leds[LED_RIGHT_EDGE_INDEX-i]-= CRGB(0, 10, 10);
    }
    for(byte i=0; i<NUM_BUTTONS; i++){
      if(!fireworks[i].isFinished()) fireworks[i].promote(leds);
    }
  }
  else {
    // mute
    fill_solid(&leds[(NUM_BUTTONS-1-((int)muteLevel))*(NUM_LEDS/2)/(NUM_BUTTONS-1)], ((int)muteLevel)*NUM_LEDS/(NUM_BUTTONS-1), CHSV(HUE_BLUE, 255,100));
  }
  FastLED.show();
}

// CODE INPUT MODE //
void runCodeInputMode(){  
  unsigned long codeInputModeTimestamp = millis();
  byte codeSize = 0;
  byte code[CODE_LENGTH] = {0};
  sendValue(77);
  enterCodeInputModeLighting();

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
    else if (codeSize == CODE_LENGTH) {
      sendCode(code);
      int msg = waitResponse();
      if (msg == 1) { 
        // valid code
        mode = PARTY_MODE;
        delay(3100); // party mode intro
        break;        
      }
      else{ 
        // invalid code
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

void enterCodeInputModeLighting(){
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  for (byte greenIndex=0; greenIndex<LED_MIDDLE_INDEX; greenIndex=greenIndex+2){
    fill_solid(&leds[LED_MIDDLE_INDEX-greenIndex], 2*greenIndex, CRGB::Green);
    FastLED.show();
  }
  fill_solid(leds, NUM_LEDS, CHSV(HUE_PURPLE,255,60));
  for (byte i=0; i<NUM_BUTTONS; i++){
    fill_solid(&leds[i*30+10], i, CRGB::White);
  }
  FastLED.show();
}



// -- METHODS -- //

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
  for(byte i=0; i<CODE_LENGTH; i++){
    Serial.write(code[i]+48);
  }
  Serial.flush();
}

int waitResponse(){
  while(!Serial.available()){}
  int msg = Serial.read();
  return msg;
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
