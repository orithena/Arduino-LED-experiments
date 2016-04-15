
#define IR_PIN  2  // IR Sensor Input Pin
#define IR_INT  0  // IR Sensor Pin Interrupt Number
// IR Timing constants: microseconds -- See also: Extended NEC IR Protocol
#define IR_INTRO_H_L 8400  // IR Prefix High Length, Lower bound
//#define IR_INTRO_H_U 42000  // IR Prefix High Length, Upper bound
#define IR_INTRO_H_U 10000  // IR Prefix High Length, Upper bound
#define IR_INTRO_L_L 3600  // IR Prefix Low Timing, Lower bound
//#define IR_INTRO_L_U 12000  // IR Prefix Low Timing, Upper bound
#define IR_INTRO_L_U 5200  // IR Prefix Low Timing, Upper bound
#define IR_BITPULSE_L 450  // IR Bit Intro Pulse Length, Lower bound
#define IR_BITPULSE_U 800  // IR Bit Intro Pulse Length, Upper bound
#define IR_BIT_0_L 350  // IR 0-Bit Pause Length, Lower bound
#define IR_BIT_0_U 800  // IR 0-Bit Pause Length, Upper bound
#define IR_BIT_1_L 1400  // IR 1-Bit Pause Length, Lower bound
#define IR_BIT_1_U 1900  // IR 1-Bit Pause Length, Upper bound

volatile unsigned long IR_lastmicro = 0;
volatile unsigned int IR_buffer[64];
#ifdef DEBUGIR
volatile unsigned int IR_error_buffer[64];
#endif
volatile unsigned int IR_pulse_len = 0;
volatile unsigned int IR_pulse_lastlen = 0;
volatile short IR_buf_pos = -1;  
volatile unsigned long IR_code = 0;

void IR_decode() {
  int len_even_H, len_odd_L, b;
  IR_code = 0;
  #ifdef DEBUGIR
  for( int n = 0; n < 64; n++ ) {
    IR_error_buffer[n] = IR_buffer[n];
  }
  #endif

  for( int a = 31; a >= 0; a-- ) {
    b = a*2;
    len_even_H = IR_buffer[b];
    len_odd_L  = IR_buffer[b + 1];
    if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_0_L,len_odd_L,IR_BIT_0_U) ) {
      //pass
    } 
    else if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_1_L,len_odd_L,IR_BIT_1_U) ) {
      IR_code = IR_code | (unsigned long)1 << (31-a);
    } 
    else if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && a >= 31 && IR_BIT_1_L < len_odd_L ) {
      IR_code = IR_code | (unsigned long)1 << (31-a);
    } 
    else {
      IR_code = 0xFFFFFFFFL;
      return;
    }
  }
}

void IR_record() {
  unsigned long micro = micros();
  IR_pulse_len = int(micro - IR_lastmicro);
  if( IR_buf_pos > 63 ) {
    IR_buf_pos = -1;
    noInterrupts();
    IR_decode();
    interrupts();
  } 
  else if( IR_buf_pos >= 0 ) {
    IR_buffer[IR_buf_pos++] = IR_pulse_len;
  } 
  else if( between(IR_INTRO_H_L,IR_pulse_lastlen,IR_INTRO_H_U) && between(IR_INTRO_L_L,IR_pulse_len,IR_INTRO_L_U) ) {
    IR_buf_pos = 0;
  } 
  IR_lastmicro = micro;
  IR_pulse_lastlen = IR_pulse_len;
}

void NEC_IR_setup() {
  pinMode(IR_PIN, INPUT);
  attachInterrupt(IR_INT, IR_record, CHANGE);
}

boolean IR_receiving() {
  return IR_buf_pos >= 0;
}

unsigned long NEC_IR_readlastcode() {
  unsigned long ret = IR_code;
  if( IR_code > 0 ) {
  #ifdef DEBUGIR
    Serial.println();
    Serial.println("Errorbuf");
    for( int n = 0; n < 32; n++ ) {
      int m = n*2;
      Serial.print(" ");
      Serial.print(IR_error_buffer[m]);
      Serial.print(",");
      Serial.print(IR_error_buffer[m+1]);
    }
  #endif
    IR_code = 0;
  }
  return ret;
}
