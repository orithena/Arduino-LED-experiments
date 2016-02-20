
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

int pin = 2;
volatile unsigned long micro = 0;
volatile unsigned long lastmicro = 0;

volatile unsigned int buffer[64];
volatile unsigned int len = 0;
volatile unsigned int lastlen = 0;
volatile short pos = -1;  
volatile unsigned long lastcode = 0;


void setup() {
  Serial.begin(115200);
  pinMode(pin, INPUT);
  attachInterrupt(0, record, CHANGE);
}

void loop() {
  if( lastcode > 0 ) {
    for( int a = 0; a < 64; a++ ) {
      Serial.print(buffer[a]); 
      Serial.print(" ");
    }
    Serial.println();
    Serial.println(lastcode, HEX);
    lastcode = 0;
  }
  delay(50);
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
    if( between(IR_BITPULSE_L,buffer[b],IR_BITPULSE_U) && between(IR_BIT_0_L,buffer[b1],IR_BIT_0_U) ) {
      //pass
    } 
    else if( between(IR_BITPULSE_L,buffer[b],IR_BITPULSE_U) && between(IR_BIT_1_L,buffer[b1],IR_BIT_1_U) ) {
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
  else if( between(IR_INTRO_H_L,lastlen,IR_INTRO_H_U) && between(IR_INTRO_L_L,len,IR_INTRO_L_U) ) {
    pos = 0;
  } 
  lastmicro = micro;
  lastlen = len;
}

