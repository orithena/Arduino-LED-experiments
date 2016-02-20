int pin = 2;
volatile unsigned long micro = 0;
volatile unsigned long lastmicro = 0;
volatile unsigned int len = 0;
volatile unsigned int lastlen = 0;
volatile byte pos = 0;
volatile unsigned long lastcode = 0;

void setup() {
    Serial.begin(9600);
    pinMode(pin, INPUT);
    attachInterrupt(0, record, CHANGE);
}

void loop() {
  if( pos == 0 && lastcode > 0 ) {
    Serial.println(lastcode, HEX);
    lastcode = 0;
  }
  delay(50);
}

inline boolean between(int a, int x, int b) {
  return (a < x) && (x < b);
}

void record() {
  micro = micros();
  len = int(micro - lastmicro);
  if( between(8800,lastlen,9600) && between(4000,len,4800) ) {
    pos = 31;
    lastcode = 0;
  } else if( pos > 0 ) {
    if( between(680,lastlen,750) && between(400,len,450) ) {
      pos--;
    } else if( between(680,lastlen,750) && between(1525,len,1625) ) {
      lastcode = lastcode | 1<<pos--;
    }
  }
  lastmicro = micro;
  lastlen = len;
}
