void setup() {
  // put your setup code here, to run once:
  pinMode(5,INPUT);
  pinMode(6,INPUT);
  pinMode(7,INPUT);
  Serial.begin(115200);
  Serial.println("Starting logger");
}

byte pins[3];
byte oldpins[3];

void loop() {
  // put your main code here, to run repeatedly:
  boolean pinchanged = false;
  for( int a = 0; a < 3; a++ ) {
    pins[a] = digitalRead(5+a);
    if( oldpins[a] != pins[a] ) {
      pinchanged = true;
    }
    oldpins[a] = pins[a];
  }
  if( pinchanged ) {
    Serial.print(millis());
    Serial.print("\t");
    Serial.print(pins[0]);
    Serial.print("\t");
    Serial.print(pins[1]);
    Serial.print("\t");
    Serial.println(pins[2]);
  }
}
