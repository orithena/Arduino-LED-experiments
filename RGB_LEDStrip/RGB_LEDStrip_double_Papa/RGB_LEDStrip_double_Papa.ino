//#define DEBUG_COLORS
#define RED      D5 // pin for red LED
#define GREEN    D6 // pin for green
//#define RED      D2 // pin for red LED
//#define GREEN    D5 // pin for green
#define BLUE     D2 // pin for blue
#define RED_FACTOR    0.96    // RED correction factor to compensate for different LED brightnesses
#define GREEN_FACTOR  1.0    // GREEN correction factor
#define BLUE_FACTOR   0.9375     // BLUE correction factor. At least one of these factors should be 1.0 and none must be greater than 1.0

#define IR_PIN  D7  // IR Sensor Input Pin
#define IR_INT  digitalPinToInterrupt(D7)  // IR Sensor Pin Interrupt Number
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
#define RISINGSUN 6

volatile unsigned long lastmicro = 0;
volatile unsigned int buffer[64];
volatile unsigned int lastlen = 0;
volatile short pos = -1;  
volatile unsigned long lastcode = 0;

double oh = 0, os = 0, ov = 0; // "old" hue, saturation, value
double ch = 0, cs = 0, cv = 0; // "current" hue, saturation, value
double th = 0, ts = 0, tv = 0; // "target" hue, saturation, value
uint32_t steps = 0, cstep = 0;   // steps to take in this scene, current step
boolean power = true;       // Have you tried turning it off and on again?
uint8_t scenecounter = 0;       // counts the scenes inside a mode (scene 0 = intro, 1..n = cycle)
// Internal user state variables -- set directly to initialize your debug settings
uint16_t usr_hue = 0;
uint8_t  usr_sat = 0, usr_val = 255;
uint8_t  usr_mode = RISINGSUN, usr_submode = 0; 
uint8_t  usr_speed = 2;
boolean need_to_save = false;
#define MAGIC 0xDA

#include <EEPROM.h>
#define FREQUENCY    160                  // valid 80, 160
#include "ESP8266WiFi.h"
extern "C" {
#include "user_interface.h"
}

void setup() {
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(2);                                // give RF section time to shutdown
  system_update_cpu_freq(FREQUENCY);
  pinMode(RED, OUTPUT);     
  pinMode(GREEN, OUTPUT);     
  pinMode(BLUE, OUTPUT);     
  pinMode(IR_PIN, INPUT);
  attachInterrupt(IR_INT, record, CHANGE);
#ifdef DEBUG_COLORS
  Serial.begin(115200);
  Serial.println("======================= START!");
#endif
  setmode(RISINGSUN);
  EEPROM.begin(8);
  read_settings();
  analogWriteFreq(4095);
  //fadeTo(0,0,0,1);
  randomSeed(analogRead(0));
}

void loop() {
  if( lastcode > 0 ) {
#ifdef DEBUG_COLORS
    Serial.println(lastcode, HEX);
#endif
    button(lastcode);
    lastcode = 0;
  }
  if(cstep > steps) {
    moderunner();
  } 
  else {
    fader();
  }
  wdt_reset();
  if( need_to_save && micros() - lastmicro > 10000000 ) {
    save_settings();
    need_to_save = false;
  }
  delay(1);
}

inline void moderunner() {
  uint16_t cycles;
  scenecounter++;
#ifdef DEBUG_COLORS
  Serial.print(usr_mode);
  Serial.print("\t");
  Serial.print(usr_submode);
  Serial.print("\t");
  Serial.print(scenecounter);
  Serial.print("\t");
  Serial.print(usr_val);
  Serial.print("\t");
  Serial.print(usr_speed);
  Serial.print("\t");
  Serial.print(cstep);
  Serial.print("\t");
  Serial.println(steps);
#endif
  switch(usr_mode) {
  case FLASH:
    cycles = 2047 >> usr_submode;
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
      fadeTo(usr_hue,usr_sat,255,1); 
      break;
    default: 
      fadeWait(cycles); 
      scenecounter = 0; 
      break;
    }
    break;
  case STROBE:
    if(usr_submode == 0) {
      fadeTo(usr_hue, usr_sat, 255, 24);
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
        fadeTo(usr_hue,usr_sat,255,1); 
        break;
      default: 
        fadeWait(10*usr_submode); 
        scenecounter = 0; 
        break;
      }
    }
    break;
  case FADE:
    cycles = 16383 - (2048*usr_speed);
    if( scenecounter == 0 ) {
      fadeTo(ch,0,47+208*(usr_submode%2),24); 
    } 
    else {
      fadeTo(
        (usr_sat == 0.0) ? _mod((scenecounter - 1) * 157.5, 360) : usr_hue,
        47 + (208 * ((usr_submode%2) | ((scenecounter%2)^1))),
        47 + (208 * (((usr_submode%2)^1) | (scenecounter%2))),
        cycles
      );
      scenecounter %= 16;
    }
    break;
  case SMOOTH:
    cycles = 0x7FFF>>usr_speed;
    switch(scenecounter) {
    case 0: 
      fadeTo(240,255,255,24); 
      break;
    case 1: 
      fadeTo((usr_submode%2==0)?120:0,255,255,cycles); 
      break;
    case 2: 
      fadeTo((usr_submode%2==0)?0:120,255,255,cycles); 
      break;
    default: 
      fadeTo(240,255,255,cycles); 
      scenecounter = 0; 
      break;
    }
    break;
  case RANDOM:
    if( scenecounter == 1 ) {
      fadeTo(
        usr_sat == 0 ? random(360) : usr_hue + 360 + random(16)-8,
        255,
        255-random(32),
        1024+random(2048*(8-usr_speed))
      );
    } 
    else {
      fadeWait(1+random(128*(8-usr_speed)));
      scenecounter = 0;
    }
    break;
  case RISINGSUN:
    cycles = 65535 - (usr_speed * 8192);
    switch(scenecounter) {
    case 0: 
      fadeTo(25,255,128,24);
      break;
    case 1:
      fadeTo(25,192,128,cycles);
      break;
    case 2:
      fadeTo(90,32,255,cycles);
      break;
    case 3:
      fadeWait(cycles);
      break;
    case 4:
      fadeTo(30,255,192,cycles);
      break;
    case 5:
      fadeWait(cycles * 0.5);
      break;
    default:
      fadeWait(cycles * 0.25);
      scenecounter = 0; 
      break;
    break;
    }
  }
}

inline void setpower(boolean on) {
  power = on;
  need_to_save = true;
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
  need_to_save = true;
  cstep--;
}

void setspeed(int8_t new_speed) {
  if( between( 0, new_speed, 7 ) ) {
    usr_speed = new_speed;
    cstep = steps - 1;
    need_to_save = true;
  }
}

void button(uint8_t code) {
  if( code != 0xBF ) {
    setpower(true);
    need_to_save = true;
  }
  switch(code) {
  case 0x23:
    if( usr_mode == RISINGSUN ) {
      setmode(RISINGSUN);
    }
    break;
  case 0x3F:  //ON
    setmode(RISINGSUN);
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
    //setmode(FLASH);
    setspeed(usr_speed+1);
    break;
  case 0x0F:  //STROBE
    //setmode(STROBE);
    setspeed(usr_speed-1);
    break;
  case 0x17:  //SMOOTH
    //setmode(SMOOTH);
    setmode(RANDOM);
    setcolor(ch,0);
    break;
  case 0x37:  //FADE
    setmode(FADE);
    break;
  case 0xDF:  //RED
    setmode(RANDOM);
    setcolor(0,255);
    break;
  case 0x5F:  //GREEN
    setmode(RANDOM);
    setcolor(120,255);
    break;
  case 0x9F:  //BLUE
    setmode(RANDOM);
    setcolor(240,255);
    break;
  case 0x1F: //WHITE
    setmode(STROBE);
    usr_submode = 0;
    setcolor(ch,0);
    break;
  case 0xEF:  //ORANGE
    setmode(RANDOM);
    setcolor(10,255);
    break;
  case 0xCF:  //ORANGE_YELLOW
    setmode(RANDOM);
    setcolor(25,255);
    break;
  case 0xF7:  //YELLOW
    setmode(RANDOM);
    setcolor(50,255);
    break;
  case 0xD7:  //GREENISH_YELLOW
    setmode(RANDOM);
    setcolor(90,255);
    break;
  case 0x6F:  //TURQUOISE
    setmode(RANDOM);
    setcolor(145,255); 
    break;
  case 0x4F:  //CYAN
    setmode(RANDOM);
    setcolor(170,255);
    break;
  case 0x77:  //
    setmode(RANDOM);
    setcolor(210,255);
    break;
  case 0x57:  //
    setmode(RANDOM);
    setcolor(225,255);
    break;
  case 0xAF:  //BLUE_PURPLE
    setmode(RANDOM);
    setcolor(250,255);
    break;
  case 0x8F:  //DARK_PURPLE
    setmode(RANDOM);
    setcolor(275,255);
    break;
  case 0xB7:  //PINKISH_PURPLE
    setmode(RANDOM);
    setcolor(300,255);
    break;
  case 0x97:  //PINK
    setmode(RANDOM);
    setcolor(340,255);
    break;
  }
}

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

inline double linear_factor(double x, double upper) {
  return x / upper;
}

inline double quad_factor(double x, double upper) {
  return curve(linear_factor(x, upper));
}

inline double quad_fade(double old_value, double target_value, double current_step, double num_steps) {
  return old_value + ((target_value - old_value) * (quad_factor(current_step, num_steps)));
}

inline double linear_fade(double old_value, double target_value, double current_step, double num_steps) {
  return old_value + ((target_value - old_value) * (linear_factor(current_step, num_steps)));
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
  ch = _mod(linear_fade(oh, th, cstep, steps), 360);
  cs = quad_fade(os, ts, cstep, steps);
  cv = quad_fade(ov, tv, cstep, steps);

  if(power) {
    sethsv(ch, cs, cv);
  }
  else {
    ch = cs = cv = 0;
    sethsv(0,0,0);
  }
  cstep++;
}

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
      //pass, this is a zero
    } 
    else if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_1_L,len_odd_L,IR_BIT_1_U) ) {
      //this is a one, shift it to correct position.
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
  else if( between(800, lastlen, 1200) && between(400, len, 800) ) {
    lastcode = 0x23;
  }
  lastmicro = micro;
  lastlen = len;
}

inline double _abs(double x) {
  return abs(x);
  //return ((x)>0.0?(x):-(x));
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
  double rem, bd;

  val *= (1-(_abs((_mod(hue+60,120)-60)/120.0) * (sat/256)));
  hue = _mod(hue, 360);

  if (sat == 0.0) { // Acromatic color (gray).
    r=val;
    g=val;
    b=val;  
  }
  else { 
    bd = (((255 - sat) * val)/256);
    base = bd;
    rem = _mod(hue,60);
    m = ((((val-bd)*rem)/60)+bd);
    n = ((((val-bd)*(60-rem))/60)+bd);
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

double curve(double i) {
  if (i < .5) return i * i * 2;
  i = 1 - i;
  return 1 - i * i * 2;
}

void setrgb(double r, double g, double b) {
  setraw(
    r*4.0*RED_FACTOR*((double)(usr_val/256.0)), 
    g*4.0*GREEN_FACTOR*((double)(usr_val/256.0)), 
    b*4.0*BLUE_FACTOR*((double)(usr_val/256.0))
  );
}
/* void setrgb(double r, double g, double b) {
  setraw(
    r*4.0*curve(r*RED_FACTOR/256.0), 
    g*4.0*curve(g*GREEN_FACTOR/256.0), 
    b*4.0*curve(b*BLUE_FACTOR/256.0)
  );
} */
int16_t ro = 0;
int16_t go = 0;
int16_t bo = 0;

inline void setraw(int16_t r, int16_t g, int16_t b) {
  if( r > 1023 ) r = 1023;
  if( g > 1023 ) g = 1023;
  if( b > 1023 ) b = 1023;
  if( r < 1 ) r = 1;
  if( g < 1 ) g = 1;
  if( b < 1 ) b = 1;
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
#ifdef DEBUG_COLORS
  if( abs(ro-r) > 10 || abs(go-g) > 10 || abs(bo-b) > 10) {
    Serial.printf("!!! ro:%d->r:%d, go:%d->r:%d, bo:%d->b:%d\n", ro, r, go, g, bo, b);
  }
  ro=r;
  go=g;
  bo=b;
  if(_mod(cstep, steps/16) == 0) {
    Serial.print(usr_mode);
    Serial.print(" ");
    Serial.print(usr_submode);
    Serial.print(" ");
    Serial.print(scenecounter);
    Serial.print("\t");
    Serial.print(usr_val);
    Serial.print(" ");
    Serial.print(usr_speed);
    Serial.print("\t");
    Serial.print(cstep);
    Serial.print("\t");
    Serial.print(steps);
    Serial.print("\t");
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

void save_settings() {
  EEPROM.write(0, MAGIC);
  EEPROM.write(1, usr_val);
  EEPROM.write(2, usr_speed);
  EEPROM.write(3, power);
  EEPROM.commit();
}

void read_settings() {
  if( EEPROM.read(0) == MAGIC ) {
    usr_val = EEPROM.read(1);
    usr_speed = EEPROM.read(2);
    power = EEPROM.read(3);
  }
}





