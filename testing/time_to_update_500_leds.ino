// TESTING THE TIME FOR UPDATING ALL LEDS DIRECTLY FROM ARDUINO

// How many leds are in the strip?
#define NUM_LEDS 500

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 3
#define CLOCK_PIN 13

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];

// This function sets up the ledsand tells the controller about them
void setup() {
	// sanity check delay - allows reprogramming if accidently blowing power w/leds
   	delay(2000);
    Serial.begin(9600);
    Serial.println("setup.");

    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
}

// This function runs over and over, and is where you do the magic to light
// your leds.
void loop() {
   // Move a single white led 
   for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      // Turn our current led on to white, then show the leds
      leds[whiteLed] = CRGB::White;

      unsigned long t1 = millis();
      for (int i=0; i<NUM_LEDS; i++){
        leds[i] = CRGB::White;
      }
      FastLED.show();
      unsigned long t2 = millis();
      
      // Wait a little bit

      Serial.print("time to show: ");
      Serial.println(t2-t1);
      delay(2000);

      // Turn our current led back to black for the next loop around
      leds[whiteLed] = CRGB::Black;
   }
}
