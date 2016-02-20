//#define DEBUG_COLORS
#define RED      10// pin for red LED
#define GREEN    9 // pin for green
#define BLUE     11 // pin for blue
#define RED_FACTOR    1.0    // RED correction factor to compensate for different LED brightnesses
#define GREEN_FACTOR  0.5    // GREEN correction factor
#define BLUE_FACTOR   0.9375 // BLUE correction factor. At least one of these factors should be 1.0 and none must be greater than 1.0

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
#define RANDOM 5

volatile unsigned long lastmicro = 0;
volatile unsigned int buffer[64];
volatile unsigned int lastlen = 0;
volatile short pos = -1;  
volatile unsigned long lastcode = 0;

double oh = 0, os = 0, ov = 0; // "old" hue, saturation, value
double ch = 0, cs = 0, cv = 0; // "current" hue, saturation, value
double th = 0, ts = 0, tv = 0; // "target" hue, saturation, value
uint16_t steps = 0, cstep = 0;   // steps to take in this scene, current step
boolean power = true;       // Have you tried turning it off and on again?
uint8_t scenecounter = 0;       // counts the scenes inside a mode (scene 0 = intro, 1..n = cycle)
// Internal user state variables -- set directly to initialize your debug settings
uint16_t usr_hue = 0;
uint8_t  usr_sat = 0, usr_val = 255;
uint8_t  usr_mode = SMOOTH, usr_submode = 0; 

void setup() {
  for( int a=9; a<=11; a++) {
    pinMode(a, OUTPUT);     
  }
#ifndef DEBUG_COLORS
  pinMode(IR_PIN, INPUT);
  attachInterrupt(IR_INT, record, CHANGE);
#else
  Serial.begin(115200);
#endif
  fadeTo(0,0,0,1);
  randomSeed(analogRead(0));
}

void loop() {
  if( lastcode > 0 ) {
#ifdef DEBUG_COLORS
    Serial.println(lastcode, HEX);
#else
    button(lastcode);
#endif
    lastcode = 0;
  }
  if(cstep > steps) {
    moderunner();
  } 
  else {
    fader();
  }
  delay(1);
}

inline void moderunner() {
  uint16_t cycles;
  scenecounter++;
#ifdef DEBUG_COLORS
  Serial.print(usr_mode);
  Serial.print(usr_submode);
  Serial.println(scenecounter);
#endif
  switch(usr_mode) {
#ifndef DEBUG_COLORS
  case FLASH:
    cycles = 2000/(usr_submode+1);
    switch(scenecounter) {
    case 0: 
      fadeTo(ch,0,255,240); 
      break;
    case 1: 
      fadeTo(ch,0,0,1); 
      break;
    case 2: 
      fadeWait(cycles); 
      break;
    case 3: 
      fadeTo(usr_hue,usr_sat,usr_val,1); 
      break;
    default: 
      fadeWait(cycles); 
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
        fadeTo(ch,0,255,240); 
        break;
      case 1: 
        fadeTo(ch,0,0,1); 
        break;
      case 2: 
        fadeWait(10*(usr_submode+1)*usr_submode); 
        break;
      case 3: 
        fadeTo(usr_hue,usr_sat,usr_val,1); 
        break;
      default: 
        fadeWait(10*usr_submode); 
        scenecounter = 0; 
        break;
      }
    }
    break;
  case FADE:
    cycles = 4097 >> (usr_submode>>1);
    if( scenecounter == 0 ) {
      fadeTo(ch,0,255*(usr_submode%2),24); 
    } 
    else {
      fadeTo(
      (usr_sat == 0.0) ? (scenecounter - 1) * 60 : usr_hue,
      31 + (224 * ((usr_submode%2) | ((scenecounter%2)^1))),
      (usr_val * (((usr_submode%2)^1) | (scenecounter%2))),
      cycles
        );
      scenecounter %= 6;
    }
    break;
#endif
  case SMOOTH:
    cycles = 0xFFFF>>(usr_submode+1);
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
#ifndef DEBUG_COLORS
  case RANDOM:
    if( scenecounter == 1 ) {
      fadeTo(
      usr_sat == 0 ? random(360) : usr_hue + 360 + random(16)-8,
      random(192)+63,
      (random(128)+127)*(usr_val/256.0),
      random(1024+random(2048*(usr_submode+1)))
        );
    } 
    else {
      fadeWait(1+random(128*(usr_submode+1)));
      scenecounter = 0;
    }
    break;
#endif
  }
}

#ifndef DEBUG_COLORS
inline void setpower(boolean on) {
  power = on;
}

void setmode(uint8_t mode) {
  if(usr_mode != mode) {
    scenecounter = -1;
    usr_mode = mode;
    usr_submode = 0;
  } 
  else {
    scenecounter = -1;
    usr_submode = (usr_submode+1) % 4;
  }
  cstep = steps - 1;
}

void setcolor(uint16_t h, uint8_t s) {
  usr_hue = h;
  usr_sat = s;
  th = h;
  ts = s;
  cstep--;
}

void setbrightness(uint16_t v) {
  if( v > 255 ) v = 255;
  if( v < 32 ) v = 32;
  usr_val = v;
  tv = v;
  cstep--;
}

void button(uint8_t code) {
  switch(code) {
  case 0x3F:  //ON
    if( power ) {
      setmode(RANDOM);
    }
    setpower(true);
    break;
  case 0xBF:  //OFF
    setpower(false);
    break;
  case 0xFF:  //BRIGHTER
    setbrightness(usr_val + 32);
    break;
  case 0x7F:  //DARKER
    setbrightness(usr_val - 32);
    break;
  case 0x2F:  //FLASH
    setmode(FLASH);
    break;
  case 0x0F:  //STROBE
    setmode(STROBE);
    break;
  case 0x17:  //SMOOTH
    setmode(SMOOTH);
    break;
  case 0x37:  //FADE
    setmode(FADE);
    break;
  case 0xDF:  //RED
    setcolor(0,255);
    break;
  case 0x5F:  //GREEN
    setcolor(120,255);
    break;
  case 0x9F:  //BLUE
    setcolor(240,255);
    break;
  case 0x1F: //WHITE
    setcolor(ch,0);
    break;
  case 0xEF:  //ORANGE
    setcolor(10,255);
    break;
  case 0xCF:  //ORANGE_YELLOW
    setcolor(25,255);
    break;
  case 0xF7:  //YELLOW
    setcolor(50,255);
    break;
  case 0xD7:  //GREENISH_YELLOW
    setcolor(90,255);
    break;
  case 0x6F:  //TURQUOISE
    setcolor(145,255); 
    break;
  case 0x4F:  //CYAN
    setcolor(170,255);
    break;
  case 0x77:  //
    setcolor(210,255);
    break;
  case 0x57:  //
    setcolor(225,255);
    break;
  case 0xAF:  //BLUE_PURPLE
    setcolor(250,255);
    break;
  case 0x8F:  //DARK_PURPLE
    setcolor(275,255);
    break;
  case 0xB7:  //PINKISH_PURPLE
    setcolor(300,255);
    break;
  case 0x97:  //PINK
    setcolor(340,255);
    break;
  }
}
#endif
void fadeTo(uint16_t h, uint8_t s, uint8_t v, uint16_t cycles) {
  // the next three lines protect against NaN bugs. I should probably find them instead of shimming it here.
  if( ch != ch ) ch = 0;
  if( cs != cs ) cs = 0;
  if( cv != cv ) cv = 0;
  oh = ch;
  os = cs;
  ov = cv;
  th = h;
  ts = s;
  tv = v;
  steps = cycles;
  cstep = 0;
}
void fadeWait(uint16_t cycles) {
  oh = ch;
  os = cs;
  ov = cv;
  steps = cycles;
  cstep = 0;
}

double curve(double i) {
  if (i < .5) return i * i * 2;
  i = 1 - i;
  return 1 - i * i * 2;
}

double normalize(double x, double upper) {
  return x / upper;
}

void fader() {
  if( _abs(oh - th) > 180) {
    if( th < oh ) {
      th += 360;
    } 
    else {
      oh += 360;
    }
  }
  ch = _mod(oh + ((th - oh) * (normalize(cstep, steps))), 360);
  cs = os + ((ts - os) * (normalize(cstep, steps)));
  cv = ov + ((tv - ov) * (normalize(cstep, steps)));
  if(power) {
    sethsv(ch, cs, cv);
  }
  else {
    ch = cs = cv = 0;
    sethsv(0,0,0);
  }
  cstep++;
}

#ifndef DEBUG_COLORS
inline boolean between(int l, int x, int u) {
  return (unsigned)(x - l) <= (u - l);
}

inline void decode() {
  int len_even_H, len_odd_L;
  int b;
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
  unsigned int len;
  unsigned long micro = micros();
  len = micro - lastmicro;
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
#endif

inline double _abs(double x) {
  return ((x)>0.0?(x):-(x));
}

double _mod(double op, int mod) {
  return (op - (((int)(op/mod))*mod));
}

void sethsv(double hue, double sat, double val) { 
  double r;
  double g;
  double b;
  double m;
  double n;
  double base;
  double rem;

  val *= (1-(_abs((_mod(hue+60,120)-60)/120.0) * (sat/256)));

  if (sat == 0.0) { // Acromatic color (gray).
    r=val;
    g=val;
    b=val;  
  }
  else { 
    base = (((255 - sat) * val)/256);
    rem = _mod(hue,60);
    m = ((((val-base)*rem)/60)+base);
    n = ((((val-base)*(60-rem))/60)+base);
    //m = ((long(val-base)*long(hue%60))/60L)+base;
    //n = ((long(val-base)*long(60-(hue%60)))/60L)+base;
    //n = (int)(((255.0 - ((sat*rem)/60.0)) * val) / 60.0);
    //m = (int)(((255.0 - ((sat*(255.0 - rem))/60.0)) * val) / 60.0);

    switch((uint8_t)(hue/60)) {
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

    default:
      r = val;
      g = base;
      b = n;
      break;
    }
  }   
  setrgb(r,g,b);
}

void setrgb(double r, double g, double b) {
  setraw(r*RED_FACTOR*curve(r/255.0), g*GREEN_FACTOR*curve(g/255.0), b*BLUE_FACTOR*curve(b/255.0));
}

inline void setraw(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
#ifdef DEBUG_COLORS
  if(cstep%(steps/4) == 0) {
  Serial.print(ch);
  Serial.print("\t");
  Serial.print(cs);
  Serial.print("\t");
  Serial.print(cv);
  Serial.print("\tR");
  Serial.print(r);
  Serial.print("\tG");
  Serial.print(g);
  Serial.print("\tB");
  Serial.println(b);
  }
#endif
}




