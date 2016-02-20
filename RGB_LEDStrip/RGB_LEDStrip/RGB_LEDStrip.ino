//#define DEBUG
#define RED      10// pin for red LED
#define GREEN    9 // pin for green
#define BLUE     11 // pin for blue
#define RED_FACTOR    0.9375 // RED correction factor to compensate for different LED brightnesses
#define GREEN_FACTOR  0.5    // GREEN correction factor
#define BLUE_FACTOR   1.0    // BLUE correction factor. At least one of these factors should be 1.0 and none must be greater than 1.0

#define IR_PIN  2  // IR Sensor Input Pin
#define IR_INT  0  // IR Sensor Pin Interrupt Number
// IR Timing constants: microseconds -- See also: Extended NEC IR Protocol
#define IR_INTRO_H_L 8400  // IR Prefix High Length, Lower bound
#define IR_INTRO_H_U 10000  // IR Prefix High Length, Upper bound
#define IR_INTRO_L_L 3600  // IR Prefix Low Timing, Lower bound
#define IR_INTRO_L_U 5200  // IR Prefix Low Timing, Upper bound
#define IR_BITPULSE_L 450  // IR Bit Intro Pulse Length, Lower bound
#define IR_BITPULSE_U 800  // IR Bit Intro Pulse Length, Upper bound
#define IR_BIT_0_L 350  // IR 0-Bit Pause Length, Lower bound
#define IR_BIT_0_U 800  // IR 0-Bit Pause Length, Upper bound
#define IR_BIT_1_L 1400  // IR 1-Bit Pause Length, Lower bound
#define IR_BIT_1_U 1900  // IR 1-Bit Pause Length, Upper bound

// Internal Mode numbers
#define OFF 0
#define FLASH 1
#define STROBE 2
#define FADE 3
#define SMOOTH 4

volatile unsigned long micro = 0;
volatile unsigned long lastmicro = 0;
volatile unsigned int buffer[64];
volatile unsigned int len = 0;
volatile unsigned int lastlen = 0;
volatile short pos = -1;  
volatile unsigned long lastcode = 0;

int oh = 0, os = 0, ov = 0; // "old" hue, saturation, value
int ch = 0, cs = 0, cv = 0; // "current" hue, saturation, value
int th = 0, ts = 0, tv = 0; // "target" hue, saturation, value
int steps = 0, cstep = 0;   // steps to take in this scene, current step
boolean power = true;       // Have you tried turning it off and on again?
int scenecounter = 0;       // counts the scenes inside a mode (scene 0 = intro, 1..n = cycle)
// Internal user state variables:
int usr_mode = SMOOTH, usr_submode = 3, usr_hue = 0, usr_sat = 255, usr_val = 255;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  for( int a=9; a<=11; a++) {
    pinMode(a, OUTPUT);     
  }
  pinMode(IR_PIN, INPUT);
  attachInterrupt(IR_INT, record, CHANGE);
  fadeTo(0,0,0,1);
}

void loop() {
  if( lastcode > 0 ) {
#ifdef DEBUG
    Serial.println(lastcode, HEX);
#endif
    button(lastcode);
    lastcode = 0;
  }
  if(cstep > steps) {
    moderunner();
  }
  fader();
  delay(5);
}

void moderunner() {
  int cycles;
  scenecounter++;
#ifdef DEBUG
  Serial.print("mode");
  Serial.print(usr_mode);
  Serial.print("/");
  Serial.print(usr_submode);
  Serial.print("#");
  Serial.println(scenecounter);
#endif
  switch(usr_mode) {
#ifndef DEBUG
  case FLASH:
    cycles = 400/(usr_submode+1);
    switch(scenecounter) {
    case 0: 
      fadeTo(ch,0,255,24); 
      break;
    case 1: 
      fadeTo(ch,0,0,1); 
      break;
    case 2: 
      fadeTo(ch,0,0,cycles); 
      break;
    case 3: 
      fadeTo(usr_hue,usr_sat,usr_val,1); 
      break;
    default: 
      fadeTo(usr_hue,usr_sat,usr_val,cycles); 
      scenecounter = 0; 
      break;
    }
    break;
  case STROBE:
    if(usr_submode == 0) {
      fadeTo(usr_hue, usr_sat, usr_val, 24);
    } 
    else {
      switch(scenecounter) {
      case 0: 
        fadeTo(ch,0,255,24); 
        break;
      case 1: 
        fadeTo(ch,0,0,1); 
        break;
      case 2: 
        fadeWait((usr_submode+1)*usr_submode); 
        break;
      case 3: 
        fadeTo(usr_hue,usr_sat,usr_val,1); 
        break;
      default: 
        fadeWait(usr_submode); 
        scenecounter = 0; 
        break;
      }
    }
    break;
  case FADE:
    cycles = 200*(4-usr_submode);
    switch(scenecounter) {
    case 0: 
      fadeTo(ch,0,0,24); 
      break;
    case 1: 
      fadeTo(0,255,usr_val,cycles); 
      break;
    case 2: 
      fadeTo(60,255,0,cycles); 
      break;
    case 3: 
      fadeTo(120,255,usr_val,cycles); 
      break;
    case 4: 
      fadeTo(180,255,0,cycles); 
      break;
    case 5: 
      fadeTo(240,255,usr_val,cycles); 
      break;
    default: 
      fadeTo(300,255,0,cycles); 
      scenecounter = 0; 
      break;
    }
    break;
#endif
  case SMOOTH:
    cycles = 6000/(usr_submode+1);
    switch(scenecounter) {
    case 0: 
      fadeTo(240,255,usr_val,24); 
      break;
    case 1: 
      fadeTo((usr_submode%2==0)?120:0,255,usr_val,cycles); 
      break;
    case 2: 
      fadeTo((usr_submode%2==0)?0:120,255,usr_val,cycles); 
      break;
    default: 
      fadeTo(240,255,usr_val,cycles); 
      scenecounter = 0; 
      break;
    }
    break;
  }
#ifdef DEBUG
  Serial.print("-> H");
  Serial.print(th);
  Serial.print(" S");
  Serial.print(ts);
  Serial.print(" V");
  Serial.print(tv);
  Serial.print(" s");
  Serial.println(steps);
#endif
}

inline void setpower(boolean on) {
  power = on;
}

void setmode(int mode) {
  if(usr_mode != mode) {
    scenecounter = -1;
    usr_mode = mode;
    usr_submode = 0;
  } 
  else {
    scenecounter = -1;
    usr_submode = (usr_submode+1) % 4;
  }
  cstep = steps - 4;
}

void setcolor(int h, int s) {
  usr_hue = h;
  usr_sat = s;
  th = h;
  ts = s;
  cstep--;
}

void setbrightness(int v) {
  if( v > 255 ) v = 255;
  if( v < 32 ) v = 32;
  usr_val = v;
  tv = v;
  cstep--;
}

inline void brighter() {
  setbrightness(usr_val + 32);
}

inline void darker() {
  setbrightness(usr_val - 32);
}

void button(int code) {
  switch(code) {
#ifndef DEBUG
  case 0xF7C03F:  //ON
    setpower(true);
    break;
  case 0xF740BF:  //OFF
    setpower(false);
    break;
  case 0xF700FF:  //BRIGHTER
    brighter();
    break;
  case 0xF7807F:  //DARKER
    darker();
    break;
  case 0xF7D02F:  //FLASH
    setmode(FLASH);
    break;
  case 0xF7F00F:  //STROBE
    setmode(STROBE);
    break;
#endif
  case 0xF7E817:  //SMOOTH
    setmode(SMOOTH);
    break;
  case 0xF7C837:  //FADE
    setmode(FADE);
    break;
#ifndef DEBUG
  case 0xF720DF:  //RED
    setcolor(0,255);
    break;
  case 0xF7A05F:  //GREEN
    setcolor(120,255);
    break;
  case 0xF7609F:  //BLUE
    setcolor(240,255);
    break;
  case 0xF7E01F: //WHITE
    setcolor(ch,0);
    break;
  case 0xF710EF:  //ORANGE
    setcolor(10,255);
    break;
  case 0xF730CF:  //ORANGE_YELLOW
    setcolor(25,255);
    break;
  case 0xF708F7:  //YELLOW
    setcolor(50,255);
    break;
  case 0xF728D7:  //GREENISH_YELLOW
    setcolor(90,255);
    break;
  case 0xF7906F:  //TURQUOISE
    setcolor(145,255); 
    break;
  case 0xF7B04F:  //CYAN
    setcolor(170,255);
    break;
  case 0xF78877:  //
    setcolor(210,255);
    break;
  case 0xF7A857:  //
    setcolor(225,255);
    break;
  case 0xF750AF:  //BLUE_PURPLE
    setcolor(250,255);
    break;
  case 0xF7708F:  //DARK_PURPLE
    setcolor(275,255);
    break;
  case 0xF748B7:  //PINKISH_PURPLE
    setcolor(300,255);
    break;
  case 0xF76897:  //PINK
    setcolor(340,255);
    break;
#endif
  }
}

void fadeTo(int h, int s, int v, int cycles) {
  oh = ch;
  os = cs;
  ov = cv;
  th = h;
  ts = s;
  tv = v;
  steps = cycles;
  cstep = 0;
}
void fadeWait(int cycles) {
  oh = ch;
  os = cs;
  ov = cv;
  steps = cycles;
  cstep = 0;
}

void fader() {
  if( cstep <= steps ) {
    if( abs(oh - th) > 180) {
      if( th < oh ) {
        th = th + 360;
      } 
      else {
        oh = oh + 360;
      }
    }
    ch = (oh + ((long(th - oh) * (long)cstep) / steps)) % 360;
    cs = os + ((long(ts - os) * (long)cstep) / steps);
    cv = ov + ((long(tv - ov) * (long)cstep) / steps);
#ifdef DEBUG
    //if(cstep%(steps/4) == 0) {
    Serial.print(ch);
    Serial.print("\t");
    Serial.print(cs);
    Serial.print("\t");
    Serial.print(cv);
    //}
#endif
    if(power) {
      sethsv(ch, cs, cv);
    } 
    else {
      ch = cs = cv = 0;
      sethsv(0,0,0);
    }
    cstep++;
  }
}

inline boolean between(int a, int x, int b) {
  return (x >= a) && (x <= b);
}

void decode() {
  int len_even_H, len_odd_L, b;
  lastcode = 0;
  for( int a = 31; a >= 0; a-- ) {
    b = a*2;
    len_even_H = buffer[b];
    len_odd_L  = buffer[b + 1];
    if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_0_L,len_odd_L,IR_BIT_0_U) ) {
      //pass
    } 
    else if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_1_L,len_odd_L,IR_BIT_1_U) ) {
      lastcode = lastcode | (unsigned long)1 << (31-a);
    } 
    else {
      lastcode = 0xFFFFFFFFL;
      return;
    }
  }
}

void record() {
  micro = micros();
  len = int(micro - lastmicro);
  if( pos > 63 ) {
    pos = -1;
    noInterrupts();
    decode();
    interrupts();
  } 
  else if( pos >= 0 ) {
    buffer[pos++] = len;
  } 
  else if( between(IR_INTRO_H_L,lastlen,IR_INTRO_H_U) && between(IR_INTRO_L_L,len,IR_INTRO_L_U) ) {
    pos = 0;
  } 
  lastmicro = micro;
  lastlen = len;
}

void sethsv(int hue, int sat, int val) { 
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
  } 
  else { 

    if(hue > 120) {
      val *= (1-(abs(((hue+60)%120)-60)/128.0));
    }

    base = ((255 - sat) * val)>>8;
    m = ((long(val-base)*long(hue%60))/60L)+base;
    n = ((long(val-base)*long(60-(hue%60)))/60L)+base;

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

void setrgb(int r, int g, int b) {
  setraw(r*RED_FACTOR, g*GREEN_FACTOR, b*BLUE_FACTOR);
}

inline void setraw(int r, int g, int b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
#ifdef DEBUG
  //if(cstep%(steps/4) == 0) {
  Serial.print("\tR");
  Serial.print(r);
  Serial.print("\tG");
  Serial.print(g);
  Serial.print("\tB");
  Serial.println(b);
  //}
#endif
}







