/**************************************************************
//  PEDIATRIC PERIMETER ARDUINO SEGMENT FOR ADDRESSABLE LEDs
//  SRUJANA CENTER FOR INNOVATION, LV PRASAD EYE INSTITUTE
//
//  AUTHORS: Sankalp Modi, Darpan Sanghavi, Dhruv Joshi
//
//  This code gives the user the following possible LED outputs through
//  serial addressing:
//  1. Hemispheres: Turning on half of the pediatric perimeter 'sphere'
//    "h,l" for the left hemisphere
//    "h,r" for the right hemisphere
//    "h,a" for the left hemi without the central 30 degrees
//    "h,b" for the right hemi without the central 30 degrees
//
//
***************************************************************/

//NeoPixel Library from Adafruit is being used here
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define Br 25      // This is where you define the brightness of the LEDs - this is constant for all

// Declare Integer Variables for RGB values. Define Colour of the LEDs.
// Moderately bright green color.
// reducing memory usage so making these into preprocessor directives
#define r 163
#define g 255
#define b 4
#define interval 500  // this is the interval between LEDs in the sweep

#define fixationLED 12  // the fixation LED pin


/**************************************************************************************************
//
//  ARDUINO PIN CONFIGURATION TO THE LED STRIPS AND HOW MANY PIXELS TO BE TURNED ON ON EACH STRIP!!
//
//  Arduino Pin     :  16 15 3  22 21 20 34 32 30 42  44  46  48  50  52  40  38  36  28  26  24  19  18  17  11     12
//  Meridian Label  :  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  daisy  fixation
//  Meridian angle  :
//  (in terms of the isopter)
*************************************************************************************************/
byte pinArduino[] = {15, 3,  16, 22, 21, 20, 30, 32, 42, 44, 46, 48, 50, 52, 34, 36, 38, 40, 28, 19, 26, 24, 18, 17, 11};
byte numPixels[] =  {24, 24, 24, 24, 24, 11, 11, 12, 23, 23, 25, 24, 24, 26, 23, 24, 11,  9,  9,  9, 11, 26, 24, 24, 72};    // there are 72 in the daisy chain

Adafruit_NeoPixel meridians[25];    // create meridians object array for 24 meridians + one daisy-chained central strip

/*************************************************************************************************/

// THE FOLLOWING ARE VARIABLES FOR THE ENTIRE SERIALEVENT INTERRUPT ONLY
// the variable 'breakOut' is a boolean which is used to communicate to the loop() that we need to immedietely shut everything off
String inputString = "", lat = "", longit = "";
boolean acquired = false, breakOut = false, sweep = false;
unsigned long previousMillis, currentMillis;  // the interval for the sweep in kinetic perimetry (in ms)
byte sweepStart, longitudeInt, Slider = 255, currentSweepLED, sweepStrip, daisyStrip;


void setup() {
  // setup serial connection
  Serial.begin(115200);
  Serial.setTimeout(500);
  Serial.println("starting..");
  
  for(int i = 0; i < 25; i++) {
    // When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
    meridians[i] = Adafruit_NeoPixel(numPixels[i], pinArduino[i], NEO_GRB + NEO_KHZ800);
  }
  clearAll();
}

void loop() {
  if (sweep == true) {
    // we will poll for this variable and then sweep the same LED
    currentMillis = millis();
    if(currentMillis - previousMillis <= interval) {
           // update the LED to be put on. Check if the current LED is less than the length of the sweeping strip
           if (currentSweepLED >= 3) {
             // only the strip LEDs
             // lightPixelStripN(sweepStrip, currentSweepLED - 4, 0);    // put off previous LED
             lightPixelStripN(sweepStrip, currentSweepLED - 3, Br);    // put on the current LED
             meridians[sweepStrip-1].show(); // This sends the updated pixel color to the hardware.
           } else {
             // we're done with the present strip. switch to daisy chain.
             // clear all previous meridian stuff...
             meridians[sweepStrip-1].clear();
             meridians[sweepStrip-1].show();
             meridians[sweepStrip-1].begin();
             
             meridians[24].clear();
             lightPixelStripN(24, 3*daisyStrip + 2 - currentSweepLED, Br);
             meridians[24].show(); // This sends the updated pixel color to the hardware.
             meridians[24].begin();
           }
           
           // stop everything when the currentSweepLED is 0.
           if (currentSweepLED == 255) {
             previousMillis = 0; 
             clearAll();
             sweep = false;
           }
         } else {           // what to do when its within the interval
           Serial.println(currentSweepLED);    // That's the iteration of the LED that's ON 
           currentSweepLED = currentSweepLED - 1;    // update the LED that has to be on
           previousMillis = currentMillis;   
           // We notify over serial (to processing), that the next LED has come on.
         }
  }

  if (Serial.available() > 0) {
    char inChar = (char)Serial.read(); 
    // if there's a comma, that means the stuff before the comma is one character indicating the type of function to be performed
    // for historical reasons we will store this as variables (lat, long)
    if (inChar == ',') {
      breakOut = false;
      // Serial.println(inputString);
      lat = inputString;
      // reset the variable
      inputString = "";
    } else {
      // if its a newline, that means we've reached the end of the command. The stuff before this is a character indicating the second argument of the function to be performed.
      if (inChar == '\n') {
         breakOut = false;
         longit = inputString;
         // reset everything again..
         inputString = "";
        
          // we deal with 3 cases: sweeps, hemispheres and quadrants
         switch(lat[0]) {  // use only the first character, the rest is most likely garbage
             
           // this is the case of setting brightness of the LEDs
           case 'm':{
             // brightness = String(longit).toInt();
             break;
           }
           
           // change sweep time
           case 't':{
             // interval= longit.toInt();
             break;
           }
           
           // choose strip to sweep
           case 's': {
             // this is the case of sweeping a single longitude. 
             // Based on the number entered as longit[0], we will turn on that particular LED.
             byte chosenStrip = longit.toInt();
             if (chosenStrip <= 24 && chosenStrip > 0) {
               sweep = true;
               sweepStrip = chosenStrip;
               daisyStrip = daisyConverter(sweepStrip);
               currentSweepLED = numPixels[sweepStrip - 1] + 3;    // adding 3 for the 3 LEDs in the daisy chain
             }
             break;
           }
     
           case 'l':{
               // put on the fixation LED
           }
           
           case 'h': {     
             // clearAll();
             // digitalWrite(fixationLED,LOW);
             // we then switch through WHICH hemisphere
             switch(longit[0]){
               case 'l': {
                 // LEFT hemisphere.. 
                 Serial.println("left hemi");
                 hemisphere1();
                 break;
               }
               case 'r': {
                 // RIGHT hemisphere.. 
                 Serial.println("right hemi");
                 hemisphere2();             
                 break;  
               }
               // 30 degrees and outer case:
               case 'a': {
                 // 30 degrees OFF left hemisphere
                 hemisphere3();
                 break;
               }
               case 'b': { 
                 hemisphere4();
                 break;  
               }
             }
             break;
           }
           case 'q': {
             Serial.println("quadrants");
             digitalWrite(fixationLED,LOW);
             switch(longit[0]) {
               // we shall go anticlockwise. "1" shall start from the bottom right. 
              case '1': {
                quad1();
                break;
              } 
              case '2': {
                quad2();
                break;
              } 
              case '3': {
                quad3();
                break;
              } 
              case '4': {
                quad4();
                break;
              } 
              case '5': {
                // turn on only the 30 degrees and higher latitudes
                quad5();
                break;
              }
              case '6': {
                // turn on only the 30 degrees and higher latitudes
                quad6();
                break;
              }
              case '7': {
                // turn on only the 30 degrees and higher latitudes
                quad7();
                break;
              }
              case '8': {
                // turn on only the 30 degrees and higher latitudes
                quad8();
                break;
              } 
             }
             break;
           }
         } 
        
        if (longit[0] == 'x') {
          // put everything off, but put the fixation back on
          // digitalWrite(fixationLED, HIGH);
          // breakOut = true;  // break out of the loops yo
          Serial.println("clear all");
          clearAll();
          delay(1);
          // reset everything...
          sweep = false;
        } else {
          digitalWrite(fixationLED,LOW);
          inputString = "";
        }
      } else {
        inputString += inChar;
      }
    }
  }
}


/***************************************************************************************************
//
//  FUNCTION DEFINITIONS
//
***************************************************************************************************/

void clearAll() {
  // put them all off
  for(int i = 1; i <= 25; i++) {
    clearN(i);
  }
}

void clearN(int n) {
  // put off a particular meridian specified by n
    meridians[n-1].clear();
    meridians[n-1].setBrightness(0);      // because we want to "clear all"
    meridians[n-1].begin();
    meridians[n-1].show();
} 

void sphere() {
  //To draw a sphere with all the LED's on in the Perimeter, each strip is being called.
  //Pixels 25 is the strip for Daisy Chain with 72 LED's on in all.
  for(int i = 0; i < 25; i++) {
    // meridians[i].clear();
    fullStripN(i);
  }
}

//Initialises Hemisphere 1 - Left Hemisphere: Physical Meridian numbers 7 to 19.
void hemisphere1() {
  for(int i = 7; i <= 19; i++) {
    fullStripN(i);
  }
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

//Initializes Hemisphere 2 - Right Hemisphere
void hemisphere2() {
  for(int i = 1; i <= 24; i++) { 
    if( (i > 18) || (i < 8) ) { //between physical meridians 19 and 24, or 1 to 7. Not the daisy chain "meridian".
      fullStripN(i);
    }
  }
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

//Initialises Hemisphere a - Left Hemisphere without the central 30 degrees (or the central daisy)
void hemisphere3() {
  for(int i = 7; i <= 19; i++) {
    onlyStripN(i);
  }
}

//Initializes Hemisphere b - Right Hemisphere without central 30 degrees
void hemisphere4() {
  for(int i = 1; i < 24; i++) { 
    if( (i > 18) || (i < 8) ) { //between physical meridians 19 and 24, or 1 to 7. Not the daisy chain "meridian".
      onlyStripN(i);
    }
  }
}

//Initializes Quadrant 1
void quad1() {
  for(int i = 1; i <= 7; i++) { 
    fullStripN(i);
  }
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

//Initializes Quadrant 2
void quad2() {
  for(int i = 7; i <= 13; i++) { 
    fullStripN(i);
  }
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

//Initializes Quadrant 3
void quad3() {
  for(int i = 13; i <= 19; i++) { 
    fullStripN(i);
  }
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

//Initializes Quadrant 4
void quad4() {
  for(int i = 19; i <= 24; i++) { 
    fullStripN(i);
  }
  fullStripN(1);
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

//Initializes Quadrant 5 - which is quad 1 without the central 30
void quad5() {
  for(int i = 1; i <= 7; i++) { 
    onlyStripN(i);
  }
}

//Initializes Quadrant 6
void quad6() {
  for(int i = 7; i <= 13; i++) { 
    onlyStripN(i);
  }
}

//Initializes Quadrant 7 - 
void quad7() {
  for(int i = 13; i <= 19; i++) { 
    onlyStripN(i);
  }
}

//Initializes Quadrant 8
void quad8() {
  for(int i = 19; i <= 24; i++) { 
    onlyStripN(i);
  }
  onlyStripN(1);
}

void fullStripAll() {
  for(int i = 1; i <= sizeof(pinArduino); i++) {
     // turn on all strips 
    fullStripN(i);
  }
  meridians[24].show(); // This sends the updated pixel color to the hardware.
  meridians[24].begin();
}

void lightPixelStripN(int strip, int Pixel, int brightness) {
  // light up the 'n' meridian
  meridians[strip].setBrightness(brightness);
  meridians[strip].setPixelColor(Pixel, r,g,b);
  
}

void onlyStripN(int strip) {
  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
  meridians[strip-1].setBrightness(Br);
  setStripColorN(strip-1);
  meridians[strip-1].show(); // This sends the updated pixel color to the hardware.
  meridians[strip-1].begin();
}

void daisyChainN(int n){
  // first we need to convert the "real world" meridians into "daisy chain" coordinate meridians.
  int m = daisyConverter(n);
  
  // Code for lighting the appropriate LEDs for the Nth meridian. For Physical meridian 1 (j=0), Daisy strips' 1st ,2nd and 3rd LEDs are switched on.
  for(int j = 3*m; j < 3*(m + 1); j++) {
    lightPixelStripN(24, j, Br);
  }
  
}

int daisyConverter(int n) {
   // converts the given meridian into the daisy "meridian" 
   if (n < 8) {
    return 7 - n;
  } else {
    return -n + 31;
  }
}

void fullStripN(int n) {
  // this is a debugging mechanism to turn on all the strips and the daisy chain strip as well
  onlyStripN(n);
  daisyChainN(n);
}

void setStripColorN(int n) {
  // set colour on all LEDs for a strip 'n'
  for (int j = 0; j < numPixels[n]; j++) {
      meridians[n].setPixelColor(j, r,g,b);
  }
}
