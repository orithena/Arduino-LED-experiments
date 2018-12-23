
volatile byte p[3];
byte pins[256][5];
const byte espins[3] = {D5, D6, D7};

volatile byte reader = 0;
volatile byte writer = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  Serial.begin(230400);
  Serial.println("\nStarting logger");
  attachInterrupt(digitalPinToInterrupt(D5), isrD5, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D6), isrD6, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D7), isrD7, CHANGE);
  p[0] = digitalRead(D5);
  p[1] = digitalRead(D6);
  p[2] = digitalRead(D7);  
}

void loop() {
  // put your main code here, to run repeatedly:
  if( reader != writer ) {
    Serial.println();
    while ( reader != writer ) {
      reader++;
      if( pins[reader][3] == 5 ) {
        Serial.println();
      }
      Serial.print(millis());
      Serial.print("\t");
      Serial.print(pins[reader][0]);
      Serial.print("\t");
      Serial.print(pins[reader][1]);
      Serial.print("\t");
      Serial.print(pins[reader][2]);
      Serial.print("\tD");
      Serial.print(pins[reader][3]);
      Serial.print("\t");
      Serial.print(pins[reader][4]);
      Serial.print("\t");
    }
    Serial.println();
    Serial.print("---------------------------------------------------------------------------------------------------------");
  }
  if( p[0] != digitalRead(D5) ) {
    Serial.print("ED5");
    p[0] = digitalRead(D5);
  }
  if( p[1] != digitalRead(D6) ) {
    Serial.print("ED6");
    p[1] = digitalRead(D6);
  }
  if( p[2] != digitalRead(D7) ) {
    Serial.print("ED7");
    p[2] = digitalRead(D7);
  }
  delay(4321);
  Serial.print("#");
}

inline void setwriter(byte source) {
  pins[writer][4] = micros() & 0x00FF;
  pins[writer][0] = p[0];
  pins[writer][1] = p[1];
  pins[writer][2] = p[2];
//  pins[writer][0] = digitalRead(D5);
//  pins[writer][1] = digitalRead(D6);
//  pins[writer][2] = digitalRead(D7);
  pins[writer][3] = source;
}

void isrD5() {
  writer++;
  p[0] ^= 0x01;
  setwriter(5);
}
void isrD6() {
  writer++;
  p[1] ^= 0x01;
  setwriter(6);
}
void isrD7() {
  writer++;
  p[2] ^= 0x01;
  setwriter(7);
}

