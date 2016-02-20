#define DEBUG
#define RED      9// pin for red LED
#define GREEN    10 // pin for green
#define BLUE     11 // pin for blue
#define RED_FACTOR    0.6
#define GREEN_FACTOR  0.3
#define BLUE_FACTOR   1.0
#define IR_PIN  2
#define IR_INT  0

volatile unsigned long micro = 0;
volatile unsigned long lastmicro = 0;
volatile unsigned int buffer[64];
volatile unsigned int len = 0;
volatile unsigned int lastlen = 0;
volatile short pos = -1;  
volatile unsigned long lastcode = 0;

int oh = 0, os = 0, ov = 0;
int ch = 0, cs = 0, cv = 0;
int th = 0, ts = 0, tv = 0;
int steps = 0;

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
  for( int a=9; a<=11; a++) {
    pinMode(a, OUTPUT);     
  }
  pinMode(IR_PIN, INPUT);
  attachInterrupt(IR_INT, record, CHANGE);
  fadeTo(0,0,0);
}

void loop() {
  if( lastcode > 0 ) {
    switch(lastcode) {
      case 0xF7E01F: //WHITE
        fadeTo(ch,0,255);
        break;
      case 0xF720DF:  //RED
        fadeTo(0,255,255);
        break;
      case 0xF7A05F:  //GREEN
        fadeTo(120,255,255);
        break;
      case 0xF7609F:  //BLUE
        fadeTo(240,255,255);
    }
    lastcode = 0;
  }
  fader();
  delay(20);
}

void fadeTo(int h, int s, int v) {
  oh = ch;
  os = cs;
  ov = cv;
  th = h;
  ts = s;
  tv = v;
  steps = 0;
}

void fader() {
  if( steps <= 24 ) {
    if( abs(oh - th) > 180) {
      if( th < oh ) {
        th = th + 360;
      } else {
        oh = oh + 360;
      }
    }
    ch = (oh + (((th - oh) * steps) / 24)) % 360;
    cs = os + (((ts - os) * steps) / 24);
    cv = ov + (((tv - ov) * steps) / 24);
    sethsv(ch, cs, cv);
    steps++;
    #ifdef DEBUG
    Serial.print(ch);
    Serial.print(" ");
    Serial.print(cs);
    Serial.print(" ");
    Serial.println(cv);
    #endif
  }
}

inline boolean between(int a, int x, int b) {
  return (x >= a) && (x <= b);
}

void decode() {
  int b=0, b1 = 0;
  lastcode = 0;
  for( int a = 0; a < 32; a++ ) {
    b = a*2;
    b1 = b + 1;
    if( between(600,buffer[b],800) && between(350,buffer[b1],500) ) {
      //pass
    } 
    else if( between(600,buffer[b],800) && between(1500,buffer[b1],1700) ) {
      lastcode = lastcode | (unsigned long)1 << (31-a);
    } 
    else {
      lastcode = 0xFFFFFFFF;
      return;
    }
  }
}

void record() {
  micro = micros();
  len = int(micro - lastmicro);
  if( pos >= 64 ) {
    pos = -1;
    noInterrupts();
    decode();
    interrupts();
  } 
  else if( pos >= 0 ) {
    buffer[pos++] = len;
  } 
  else if( between(8800,lastlen,9600) && between(4000,len,4800) ) {
    pos = 0;
  } 
  lastmicro = micro;
  lastlen = len;
}

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
  } else { 

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
  setraw(r,g,b);
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

