
#define SENSOR_PIN 2
#define SENSOR_INT 0
#define SENSOR_THRESHOLD 100
#define SENSOR_INCREMENT 2

volatile unsigned long sensor_count = 0;
volatile unsigned long sensor_last_rising = 0;
unsigned long sensor_last_true = 0;

void sensor_setup() {
  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(SENSOR_INT, sensor_isr, RISING);
}

void sensor_isr() {
  if( sensor_count < SENSOR_THRESHOLD) {
    sensor_count += SENSOR_INCREMENT;
    sensor_last_rising = millis();
  }
}

boolean read_sensor() {
  unsigned long ms = millis();
  if( sensor_last_rising < ms - 1000 ) {
    sensor_count = 0;
  }
  if( sensor_last_true < ms - 1000 && sensor_count >= SENSOR_THRESHOLD ) {
    sensor_count = 0;
    return true;
  } else {
    return false;
  }
}
