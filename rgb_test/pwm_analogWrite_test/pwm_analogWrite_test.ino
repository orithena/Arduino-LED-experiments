#define RED       9// pin for red LED
#define GREEN    10 // pin for green
#define BLUE     11 // pin for blue
#define RED_FACTOR    1.0
#define GREEN_FACTOR  0.5
#define BLUE_FACTOR   0.9

// the setup routine runs once when you press reset:
void setup() {                
   // initialize the digital pin as an output.
   for( int a=9; a<=11; a++) {
     pinMode(a, OUTPUT);     
   }
   setrgb(255,0,0);
   delay(1000);
   setrgb(0,255,0);
   delay(1000);
   setrgb(0,0,255);
   delay(1000);
}

void setrgb(int r, int g, int b) {
  analogWrite(RED, r*RED_FACTOR);
  analogWrite(GREEN, g*GREEN_FACTOR);
  analogWrite(BLUE, b*BLUE_FACTOR);
}

void loop() {
   for(int i=0; i < 256; i++) {
     setrgb(i,0,255-i);
     delay(6);
   }
   for(int i=0; i < 256; i++) {
     setrgb(255-i,i,0);
     delay(6);
   }
   for(int i=0; i < 256; i++) {
     setrgb(0,255-i,i);
     delay(6);
   }
}
