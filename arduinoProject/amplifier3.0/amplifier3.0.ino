// ---
// --- AMPLIFIER 3.0 CODE | Yotam Shkedi (C)
// --- .this is the buttons and leds module. 
// --- .music module runs in python through serial com.
// --- 

#include <FastLED.h>
#include <avr/pgmspace.h>

#define MAIN_LEDS_NUM 300
#define MAIN_LEDS_PIN 22
#define LEG_LEDS_NUM 54
#define LEG_R_LEDS_PIN 24
#define LEG_L_LEDS_PIN 25
#define LEG_LEN 23
#define LEG_MID_LEN 8
#define BUTTONS_LIGHT_PIN 26
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
#define PARTY_MODE_SLEEPS_NUM 5
#define CODE_INPUT_MODE_DURATION 51050

#define IDLE_MODE 0b1
#define AMPLIFYING_MODE 0b10
#define PARTY_MODE 0b100
#define CODE_INPUT_MODE 0b1000

const byte KEEP_ON_CMD[CODE_LENGTH] = {9,8,8,8,9}; 
const byte TERMINATE_CMD[CODE_LENGTH] = {0,1,1,1,0}; 
const unsigned long PARTY_MODE_SLEEP_DELAYS[PARTY_MODE_SLEEPS_NUM] = {20000,10000,10000,20000,50000};
// pins of button number {0, 1, ... , 9}
byte buttonPins[NUM_BUTTONS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
bool testing = false;

// Structs //

struct buttons_state{
  byte prev[NUM_BUTTONS] = {0};
  byte current[NUM_BUTTONS] = {0};
  bool waitRelease[NUM_BUTTONS] = {false};
  byte total = 0;
  byte prevTotal= 0;
  bool wasChanged = true;
  byte lastClicks[CODE_LENGTH] = {0};
  
  void updateState(){
    prevTotal = total;
    total = 0;
    wasChanged = false;
    
    for (byte i=0 ; i < NUM_BUTTONS ; i++){
      prev[i] = current[i];
      current[i] = (digitalRead(buttonPins[i]) == LOW) ? CLICKED : UNCLICKED;
      if(current[i]==0) waitRelease[i]=false;
      if(waitRelease[i]) current[i] = UNCLICKED;
      if(current[i] != prev[i]) { wasChanged = true; }
      if (current[i]==CLICKED && prev[i]==UNCLICKED) { addClick(i); }
      total += current[i];
    }
    if(testing && current[2]){
      total = 9 + (current[0] ? 1 :0) ;
      wasChanged = current[2] != prev[2] || current[0] != prev[0];
      for (byte i=1 ; i < NUM_BUTTONS ; i++){ current[i]=CLICKED; }   
    }
  }
  
  void setWaitReleaseAll(){
    for(byte i=0; i<NUM_BUTTONS; i++) 
      if(current[i]) waitRelease[i] = true;    
  }
  void setWaitRelease(byte button){
    if(current[button]) waitRelease[button] = true;    
  }

  void addClick(byte buttonNum){
    for(byte i=0; i<CODE_LENGTH-1; i++){
      lastClicks[i] = lastClicks[i+1];
    }
    lastClicks[CODE_LENGTH-1] = buttonNum;
  }

  bool gotCommand(byte* cmd){
    for(byte i=0; i<CODE_LENGTH; i++){
      if (lastClicks[i] != cmd[i]) { return false; }
    }
    lastClicks[0] = NUM_BUTTONS+1;
    return true;
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
    m_color.setParity(reverse ? 1 : 0);
  }
  bool isFinished(){
    return m_startIndex == -1 || (m_index < -FIREWORK_LEN || m_index >= MAIN_LEDS_NUM+FIREWORK_LEN);
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
        if (m_startIndex <= m_index-i && m_index-i < MAIN_LEDS_NUM){
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
uint8_t gMainBrightness = 20;
uint8_t gLegBrightness = 50;
CRGB mainLeds[MAIN_LEDS_NUM];
CLEDController* mainController;
CRGB legLeds[LEG_LEDS_NUM];
CLEDController* legLController;
CLEDController* legRController;

byte mode;
byte sevenCounter;
unsigned long sevenTimestamp;
const PROGMEM uint16_t permutation[] = {237, 67, 253, 198, 279, 179, 247, 92, 5, 43, 41, 50, 69, 263, 149, 129, 65, 95, 54, 162, 189, 222, 42, 282, 106, 158, 18, 196, 206, 242, 167, 291, 62, 33, 102, 83, 97, 142, 16, 1, 169, 204, 230, 119, 299, 203, 235, 122, 38, 220, 141, 259, 44, 290, 232, 99, 91, 4, 185, 171, 153, 240, 246, 175, 25, 46, 75, 76, 288, 217, 226, 157, 207, 172, 271, 81, 262, 100, 128, 285, 174, 231, 272, 163, 223, 127, 239, 98, 277, 22, 229, 275, 210, 7, 202, 111, 34, 192, 147, 36, 8, 187, 298, 61, 283, 132, 32, 170, 107, 173, 178, 26, 251, 281, 82, 276, 103, 292, 68, 23, 193, 252, 9, 238, 13, 194, 114, 130, 14, 155, 136, 93, 294, 161, 236, 199, 58, 200, 268, 19, 274, 150, 84, 12, 135, 79, 265, 70, 124, 28, 250, 55, 143, 89, 215, 59, 77, 131, 156, 66, 280, 117, 243, 87, 86, 165, 164, 137, 85, 116, 60, 266, 6, 255, 40, 29, 176, 73, 0, 139, 151, 120, 195, 64, 177, 295, 11, 2, 254, 104, 10, 233, 148, 287, 17, 267, 248, 152, 113, 214, 183, 72, 191, 126, 270, 234, 39, 138, 123, 264, 118, 108, 78, 201, 180, 216, 205, 112, 227, 27, 21, 296, 53, 244, 286, 133, 166, 145, 184, 125, 52, 225, 56, 190, 257, 74, 140, 3, 241, 209, 212, 88, 105, 224, 228, 182, 273, 134, 90, 197, 121, 297, 101, 168, 24, 249, 186, 159, 109, 144, 213, 284, 245, 35, 80, 71, 47, 188, 218, 37, 260, 94, 115, 45, 146, 48, 256, 221, 160, 57, 269, 219, 49, 110, 289, 278, 31, 20, 63, 154, 258, 293, 30, 181, 96, 208, 15, 261, 211, 51};


// CODE //

void setup() {
  Serial.begin(9600);
  mainController = &FastLED.addLeds<WS2812B, MAIN_LEDS_PIN, GRB>(mainLeds, MAIN_LEDS_NUM);  // GRB ordering is typical  
  legRController = &FastLED.addLeds<WS2812B, LEG_R_LEDS_PIN, GRB>(legLeds, LEG_LEDS_NUM);  // GRB ordering is typical  
  legLController = &FastLED.addLeds<WS2812B, LEG_L_LEDS_PIN, GRB>(legLeds, LEG_LEDS_NUM);  // GRB ordering is typical  
  mainController->setDither(0);
  mainController->setCorrection(TypicalSMD5050);
  legRController->setTemperature(Tungsten40W );
  legLController->setTemperature(Tungsten40W );
  FastLED.setBrightness(20);

  for (byte i=0 ; i < NUM_BUTTONS ; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
    buttonsState.waitRelease[i] = true;
  }
  pinMode(BUTTONS_LIGHT_PIN, OUTPUT);
  digitalWrite(BUTTONS_LIGHT_PIN, HIGH);
  
  mode = IDLE_MODE;
  sevenCounter = 0;
  sevenTimestamp = 0;
  
  fill_solid(mainLeds, MAIN_LEDS_NUM, CRGB::Black);
  fill_solid(legLeds, LEG_LEDS_NUM, CRGB::Black);
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
  unsigned long idleModeStartTime = millis();
  byte hueIndex = 0;
  int ledIndex = random16(MAIN_LEDS_NUM);
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
        fadeToBlackBy(mainLeds, MAIN_LEDS_NUM, 5);
        mainController->showLeds(gMainBrightness);
        if (currentTime - idleModeStartTime > 3000){
          for(int i=0; i<LEG_LEDS_NUM; i++) legLeds[i]+=CRGB(2,2,2);
        }
        else fadeToBlackBy(legLeds, LEG_LEDS_NUM, 5);
        showLegs();
      }
    }
  }
  // exit mode
}

void updateIdleModeLighting(int &ledIndex, byte &hueIndex, bool &reverse){
  mainLeds[ledIndex].setHSV(hueIndex, 255, 255);
  if (reverse) ledIndex--; else ledIndex++;
  if (ledIndex==MAIN_LEDS_NUM-1 || ledIndex==0) reverse = !reverse;
  hueIndex += 3;
  fadeToBlackBy(mainLeds, MAIN_LEDS_NUM, 40);
  mainController->showLeds(gMainBrightness);
}

// AMPLIFYING MODE //

void runAmplifyingMode(){
  int animationIndex = 0;
  bool reverse = random8(2)==1;
    
  while(mode == AMPLIFYING_MODE){
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
      // if song ends
      int msg = Serial.read();
      mode = IDLE_MODE;
      break;
    }
    // continue running
    if(buttonsState.wasChanged){
      sendNumClicked(buttonsState.total);
      startAnimationFromAFreshlyPressedButton(animationIndex, reverse);
    }
    // lighting
    EVERY_N_MILLISECONDS(20){
      updateAmplifyingModeLighting(animationIndex, reverse);
    }
    buttonsState.updateState();
    updateSevenCounter(millis());
  }
}

void updateAmplifyingModeLighting(int &animationIndex, bool &reverse){
  uint8_t hue = beat8(24);

  // promote amination index
  byte button = animationIndex/(MAIN_LEDS_NUM/NUM_BUTTONS);
  if(buttonsState.current[button]
      && (animationIndex != button*MAIN_LEDS_NUM/NUM_BUTTONS + (reverse ? 0 : MAIN_LEDS_NUM/NUM_BUTTONS-1))){
    // if there is a way in the current button's window
    animationIndex = reverse ? animationIndex-1 : animationIndex+1;
  }
  else{
    byte nextActiveButton = findNextActiveButtonAndDirection(button, reverse);
    if (nextActiveButton != BUTTON_NOT_EXIST){
      animationIndex = nextActiveButton*MAIN_LEDS_NUM/NUM_BUTTONS + (reverse ? MAIN_LEDS_NUM/NUM_BUTTONS-1 : 0);
    }
  }
  // set mainLeds
  mainLeds[animationIndex] = CHSV(hue,255,255);

  for(byte i=0; i<NUM_BUTTONS; i++){
    if (buttonsState.current[i] == UNCLICKED){
      fill_solid(&mainLeds[i*30], 30, CRGB::Black);
      if (beat8(20)>240) mainLeds[i*30+15] = CRGB::OrangeRed;
    }
  }
  mainController->showLeds(gMainBrightness);
  if(buttonsState.wasChanged){
    byte lightPart = LEG_LEN * buttonsState.total/NUM_BUTTONS;
    fill_solid(legLeds, LEG_LEDS_NUM, CRGB::Black);
    fill_solid(&legLeds[LEG_LEN-lightPart], lightPart, CRGB::White);
    fill_solid(&legLeds[LEG_LEN+LEG_MID_LEN], lightPart, CRGB::White);
    if (lightPart > LEG_MID_LEN) {
      fill_solid(&legLeds[LEG_LEN], LEG_MID_LEN, CRGB::White);
    }
    showLegs();
  }
}

void startAnimationFromAFreshlyPressedButton(int &animationIndex, bool reverse){
  // start from a recent-clicked-button
  uint8_t hue = beat8(24);
  for (byte i=0; i<NUM_BUTTONS; i++){
    byte buttonNumber = reverse ? NUM_BUTTONS-1-i : i;
    if (buttonsState.current[buttonNumber] == CLICKED 
      && buttonsState.prev[buttonNumber] == UNCLICKED) {
      fill_rainbow(&mainLeds[buttonNumber*MAIN_LEDS_NUM/NUM_BUTTONS+7], 14, (reverse ? hue : hue-14*5), (reverse ? -5 : 5));
      animationIndex = buttonNumber*(MAIN_LEDS_NUM/NUM_BUTTONS) + (reverse ? 7 : 20);  
    }
  }
}



// PARTY MODE //

void runPartyMode(){
  byte startAnimation = 0;
  byte muteLevel = 0;
  digitalWrite(BUTTONS_LIGHT_PIN, LOW);
  buttonsState.setWaitReleaseAll();
  Firework fireworks[NUM_BUTTONS];
  int permutationIndex = 0;
  byte sleepPhaseNum = 0;
  unsigned long sleepUntil = millis()+PARTY_MODE_SLEEP_DELAYS[sleepPhaseNum];
  sleepPhaseNum++;
  
  while(mode == PARTY_MODE){
    if(Serial.available()){
      int msg = Serial.read();
      mode = IDLE_MODE;
      break;
    }
    // continue running
    buttonsState.updateState();

    // add fireworks
    for(byte i=1; i<NUM_BUTTONS-1; i++){
      if(buttonsState.current[i]==CLICKED && buttonsState.prev[i]==UNCLICKED && fireworks[i].isFinished()){
        fireworks[i].initialize((int)(MAIN_LEDS_NUM/NUM_BUTTONS*i+15), (CRGB)CRGB::White, random8(2)==0); 
      }
    }
    unsigned long currentTime = millis();
    if(currentTime > sleepUntil){
       EVERY_N_MILLISECONDS(500) {
        digitalWrite(BUTTONS_LIGHT_PIN, !digitalRead(BUTTONS_LIGHT_PIN));
      }
      for (byte i=0; i < NUM_BUTTONS; i++){
        if(buttonsState.current[i]==CLICKED) { 
          if (muteLevel > 0){
            startAnimation = 0;
            muteLevel = 0;
            sendValue(muteLevel+20);
          }
          if (sleepPhaseNum < PARTY_MODE_SLEEPS_NUM){
            sleepUntil = currentTime + PARTY_MODE_SLEEP_DELAYS[sleepPhaseNum];
            sleepPhaseNum++;
          }
          else { sleepUntil = currentTime + 600000; }
          digitalWrite(BUTTONS_LIGHT_PIN, LOW);
          permutationIndex = 0;
          break;
        }
      }

      if (currentTime > sleepUntil + 10000){
        EVERY_N_MILLISECONDS(65) {
          permutationIndex = (permutationIndex+1)%MAIN_LEDS_NUM ;
        }
        EVERY_N_MILLISECONDS(2000) {
          muteLevel++;
          sendValue(muteLevel+20);
        }
      }
      if (muteLevel == NUM_BUTTONS){
        mode = IDLE_MODE;
        break;        
      }
    }
    else {
      for (byte i=0; i < NUM_BUTTONS; i++){
        if(buttonsState.current[i]==CLICKED) {
          unsigned long currentDelay = 
            (sleepPhaseNum < PARTY_MODE_SLEEPS_NUM) ? 
            PARTY_MODE_SLEEP_DELAYS[sleepPhaseNum] : 600000;
          if (sleepUntil < currentTime+currentDelay) { sleepUntil = currentTime+currentDelay; }
        }
      }
      if (buttonsState.gotCommand(KEEP_ON_CMD)) { 
        sleepUntil += 600000; 
        }
      if (buttonsState.gotCommand(TERMINATE_CMD)) {
        sleepUntil = currentTime - 10001;
        muteLevel = 7;
        sleepPhaseNum = 1;
        buttonsState.setWaitReleaseAll();
      }
    }
    
    // lighting
    updatePartyModeLighting(startAnimation, permutationIndex, muteLevel, fireworks);
    
  }
  // exit party mode
  buttonsState.setWaitReleaseAll();
  digitalWrite(BUTTONS_LIGHT_PIN, HIGH);
}

void setPartyModeRainbow(){
  byte pattern = beat8(1) / (255/3);
  static byte hue = 0;
  static byte index = 0;
  if (pattern == 0){ 
    fill_rainbow(mainLeds, MAIN_LEDS_NUM, beatsin8(7)+beatsin8(19)+beatsin8(53), 5);
  }
  else if (pattern == 1) { 
    for (byte i=0; i<NUM_BUTTONS; i++){
      fill_solid(&mainLeds[MAIN_LEDS_NUM/NUM_BUTTONS*i], MAIN_LEDS_NUM/NUM_BUTTONS, CHSV(2*255/NUM_BUTTONS*i+beat8(30),200,255));
    }
  }  
  else if (pattern == 2) {
    byte nextHue = hue + 157;
    for (byte i=0; i<NUM_BUTTONS; i++){
      fill_solid(&mainLeds[MAIN_LEDS_NUM/NUM_BUTTONS*i], MAIN_LEDS_NUM/NUM_BUTTONS, CHSV(hue+10*i, 255,255));
      fill_solid(&mainLeds[MAIN_LEDS_NUM/NUM_BUTTONS*i+15-index], index*2, CHSV(nextHue+10*i,255,255));
    }
    EVERY_N_MILLISECONDS(50){
      if (index == 15) { 
        index = 0;
        hue = nextHue; 
      }
      else { index++; }
    }
  }
  //fill_rainbow(&mainLeds[LED_MIDDLE_INDEX], MAIN_LEDS_NUM/2-rightMuteIndex, beat8(40), -3);
  //fill_rainbow(&mainLeds[LED_LEFT_EDGE_INDEX+leftMuteIndex], MAIN_LEDS_NUM/2-leftMuteIndex, beat8(40)-3*(MAIN_LEDS_NUM/2-leftMuteIndex), 3);
}

void updatePartyModeLighting(byte &startAnimation, int &permutationIndex, byte muteLevel, Firework* fireworks){
  if (startAnimation < MAIN_LEDS_NUM/2){
      setPartyModeRainbow();
      fill_solid(mainLeds, MAIN_LEDS_NUM/2-startAnimation, CRGB::Black);
      fill_solid(&mainLeds[MAIN_LEDS_NUM/2+startAnimation], MAIN_LEDS_NUM/2-startAnimation, CRGB::Black);
      mainController->showLeds(gMainBrightness);
      startAnimation+=2;
      EVERY_N_MILLISECONDS(50){
        fill_solid(legLeds, LEG_LEDS_NUM, CRGB::Black);
        byte lightPart = startAnimation/2.5;
        if(lightPart < LEG_LEN){
          fill_solid(legLeds, lightPart, CRGB::White);
        }
        else{
          fill_solid(legLeds, LEG_LEN, CRGB::White);
          if(lightPart < LEG_LEN*2){
            fill_solid(&legLeds[2*LEG_LEN+LEG_MID_LEN-1-lightPart+LEG_LEN], lightPart-LEG_LEN, CRGB::White);
          }
          else{
            fill_solid(&legLeds[LEG_LEN+LEG_MID_LEN], LEG_LEN, CRGB::White);
            fill_solid(&legLeds[LEG_LEN], lightPart-2*LEG_LEN, CRGB::White);
          }
        }
        showLegs();
      }
    }
  else if (startAnimation < MAIN_LEDS_NUM/2 + 8){
    EVERY_N_MILLISECONDS(20) {
      if (startAnimation % 2 == 0) {
        setPartyModeRainbow();
        fill_solid(legLeds, LEG_LEDS_NUM, CRGB::White);
      }
      else {
        fill_solid(mainLeds, MAIN_LEDS_NUM, CRGB::Black); 
        fill_solid(legLeds, LEG_LEDS_NUM, CRGB::Black); 
      }
      mainController->showLeds(gMainBrightness);
      showLegs();
    }
    EVERY_N_MILLISECONDS(400){
      // FOR BLINKING AFTER START ANIMATION
      startAnimation++;  
    }  
  }
  else {
    EVERY_N_MILLISECONDS(20){
      if (muteLevel == 0){
        setPartyModeRainbow();  
        for(byte i=0; i<NUM_BUTTONS; i++){
          if(!fireworks[i].isFinished()) fireworks[i].promote(mainLeds);
        }
        fill_solid(legLeds, LEG_LEDS_NUM, CHSV(beat8(5), 120, 255));
        showLegs();
      }
      else {
        // mute
        uint16_t i = pgm_read_word(&permutation[permutationIndex]);
        mainLeds[i] = CRGB::Black;
      }
      mainController->showLeds(gMainBrightness);
    }
  }
}




// CODE INPUT MODE //

void runCodeInputMode(){  
  unsigned long codeInputModeTimestamp = millis();
  byte codeSize = 0;
  byte code[CODE_LENGTH] = {0};
  sendValue(77);
  enterCodeInputModeLighting();
  setCodeInputModeButtonsLighting();

  while(mode == CODE_INPUT_MODE){
    delay(100);
    buttonsState.updateState();
    bool failed = false;
    updateCodeInput(code, codeSize, codeInputModeTimestamp, failed);
    if(failed){
      sendValue(88);
      fill_solid(mainLeds, MAIN_LEDS_NUM, CRGB::Red);
      mainController->showLeds(gMainBrightness);
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
        fill_solid(mainLeds, MAIN_LEDS_NUM, CRGB::Red);
        mainController->showLeds(gMainBrightness);
        delay(3000); // failed sound
        break;  
      }
    }
    // continue running
    // lighting
  }
  buttonsState.updateState();
  buttonsState.setWaitReleaseAll();
}

void enterCodeInputModeLighting(){
  fill_solid(mainLeds, MAIN_LEDS_NUM, CRGB::Black);
  mainController->showLeds(gMainBrightness);
  
  for (byte greenIndex=0; greenIndex<LED_MIDDLE_INDEX; greenIndex=greenIndex+2){
    fill_solid(&mainLeds[LED_MIDDLE_INDEX-greenIndex], 2*greenIndex, CRGB::Green);
    mainController->showLeds(gMainBrightness);
  }
  fill_solid(legLeds, LEG_LEDS_NUM, CHSV(HUE_BLUE+10, 150, 255));
  showLegs();
}

void setCodeInputModeButtonsLighting(){
  fill_solid(mainLeds, MAIN_LEDS_NUM, CHSV(HUE_PURPLE,255,60));
  for (byte i=0; i<NUM_BUTTONS; i++){
    fill_solid(&mainLeds[i*30+15], i, CRGB::White);
  }
  mainController->showLeds(gMainBrightness);
}


// -- METHODS -- //

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

void updateCodeInput(
    byte code[], 
    byte &codeSize, 
    unsigned long codeInputModeTimestamp, bool &failed){
  unsigned long currentTime = millis();
  if(currentTime - codeInputModeTimestamp > CODE_INPUT_MODE_DURATION){
    failed = true;
    return;
  }
  bool reset = false;
  for (byte i=0; i < codeSize ; i++){
    if(buttonsState.prev[code[i]] && buttonsState.current[code[i]]==UNCLICKED){
      reset = true;
    }
  }
 
  boolean change = false;
  for (byte i=0; i<NUM_BUTTONS ; i++){
    if(buttonsState.prev[i]==UNCLICKED && buttonsState.current[i]){
      sendValue(i+65);
      if(change){
        reset = true;
      }
      change = true;
      code[codeSize] = i;
      fill_solid(&mainLeds[i*30], 29, CHSV(HUE_GREEN-20*codeSize, 255, 255));
      codeSize++;
      mainController->showLeds(gMainBrightness);
    }
  }
  if (reset){
    codeSize = 0;
    fill_solid(mainLeds, MAIN_LEDS_NUM, CHSV(HUE_RED, 150, 255));
    mainController->showLeds(gMainBrightness);
    delay(1000);
    setCodeInputModeButtonsLighting();
    buttonsState.setWaitReleaseAll();
  }
}

void updateSevenCounter(unsigned long currentTime){
  if(currentTime - sevenTimestamp > SEVEN_SPEED){
    sevenCounter = 0;
  }
  if(buttonsState.current[7] == CLICKED && buttonsState.prev[7]==UNCLICKED){
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
  if(testing){
    for (byte i=0; i<NUM_BUTTONS; i++){
      Serial.write(buttonsState.current[i]+30);
    }
    Serial.flush();
  }  
  delay(50);
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

void fillLeds(int from, int to, CRGB value){
  for (int i=from; i < to; i++){
    mainLeds[i] = value;
  }
}

void showLegs(){
  legLController->showLeds(gLegBrightness);
  legRController->showLeds(gLegBrightness);
}
