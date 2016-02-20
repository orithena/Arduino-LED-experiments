#define RED       9// pin for red LED
#define GREEN    10 // pin for green
#define BLUE     11 // pin for blue
#define RED_FACTOR    0.6
#define GREEN_FACTOR  0.3
#define BLUE_FACTOR   1.0

void sethsv(int hue, int sat, int val) { 
  hue = hue % 360;
  int r;
  int g;
  int b;
  int m;
  int n;
  int base;

  if (sat == 0) { // Acromatic color (gray).
    r=val;
    g=val;
    b=val;  
  } else  { 

    val *= (1-(abs(((hue+60)%120)-60)/128.0));
    base = ((255 - sat) * val)>>8;
    m = (((val-base)*(hue%60))/60)+base;
    n = (((val-base)*(60-(hue%60)))/60)+base;

    switch(hue/60) {
	case 0:
		r = val;
		g = m;
		b = base;
	break;

	case 1:
		r = n;
		g = val;
		b = base;
	break;

	case 2:
		r = base;
		g = val;
		b = m;
	break;

	case 3:
		r = base;
		g = n;
		b = val;
	break;

	case 4:
		r = m;
		g = base;
		b = val;
	break;

	case 5:
		r = val;
		g = base;
		b = n;
	break;
    }
  }   
  setrgb(r,g,b);
}


// the setup routine runs once when you press reset:
void setup() {                
   for( int a=9; a<=11; a++) {
     pinMode(a, OUTPUT);     
   }
   setraw(255,255,255);
   delay(2000);
   setrgb(0,0,0);
   delay(20);
   setrgb(255,255,255);
   delay(2000);
   setrgb(0,0,0);
   delay(20);
   sethsv(0,0,255);
   delay(2000);
   setrgb(0,0,0);
   delay(200);
}

void setrgb(int r, int g, int b) {
  analogWrite(RED, r*RED_FACTOR);
  analogWrite(GREEN, g*GREEN_FACTOR);
  analogWrite(BLUE, b*BLUE_FACTOR);
}
void setraw(int r, int g, int b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}

void loop() {
 for(int k=4; k > 0; k--) {
  for(int j=6; j > 0; j--) {
   for(int i=0; i < 600; i++) {
     sethsv(i,(k*64)-1,(j*42)-1);
     delay(8);
   }
   setrgb(0,0,0);
   delay(1000);
  }
  setrgb(255,255,255);
  delay(2000);
 }
}


