#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_RMT_BUILTIN_DRIVER 0
//#define FASTLED_RMT_MAX_CHANNELS 1
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#define FASTLED_SHOW_CORE 1
#define FASTLED_USE_TASK

//#define DEBUG

#define DATA_PIN    22
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define ROWS 1
#define COLS 80
#define NUM_LEDS    (ROWS*COLS)
CRGB leds[NUM_LEDS];

// Replace with your network credentials
const char* ssid     = "WallCircle";

bool update_lights = true;
bool update_display = true;

int freq = 32768;
int topChannel = 0;
int bottomChannel = 1;
int resolution = 8;

inline int ui_to_val(int x) {
  return square(x/100.0) * 255.0;
}

inline int val_to_ui(int x) {
  return sqrt(x / 255.0) * 100;
}

inline uint16_t XY( uint16_t x, uint16_t y)
{
  return (((ROWS-1-y) * COLS) + ( (y & 0x01) == 0 ? (COLS - 1) - x : x ) ) % NUM_LEDS;
}

#define LED(_X,_Y) leds[XY(_X,_Y)]

typedef struct State {
  int light;
  float ratio;
  uint8_t display_mode;
  bool display_static;
} State;

State state = {
  light: ui_to_val(100),
  ratio: 0.25,
  display_mode: 0,
  display_static: false
};


#define SHORT_TOUCH 16
#define MIN_TOUCH 5
#define LIGHTEN_THROTTLE 2

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef int (*SimplePatternList[])();
//SimplePatternList gPatterns = { juggle, plain, sinefield2, sinematrix3 };
SimplePatternList gPatterns = { juggle };

#ifdef FASTLED_USE_TASK
// -- Task handles for use in the notifications
static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

/** show() for ESP32
 *  Call this function instead of FastLED.show(). It signals core 0 to issue a show, 
 *  then waits for a notification that it is done.
 */
void FastLEDshowESP32()
{
    if (userTaskHandle == 0) {
        // -- Store the handle of the current task, so that the show task can
        //    notify it when it's done
        userTaskHandle = xTaskGetCurrentTaskHandle();

        // -- Trigger the show task
        xTaskNotifyGive(FastLEDshowTaskHandle);

        // -- Wait to be notified that it's done
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
        ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        userTaskHandle = 0;
    }
}

/** show Task
 *  This function runs on core 0 and just waits for requests to call FastLED.show()
 */
void FastLEDshowTask(void *pvParameters)
{
    // -- Run forever...
    for(;;) {
        // -- Wait for the trigger
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // -- Do the show (synchronously)
        FastLED.show();

        // -- Notify the calling task
        xTaskNotifyGive(userTaskHandle);
    }
}
#endif

void setup() {
  
  Serial.begin(115200);
  Serial.printf("\n\nPowerLight is booting...\n\nClock Speed: ");

  setup_display();

}

void setup_display() {
  pinMode(DATA_PIN, OUTPUT);
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);

  #ifdef FASTLED_USE_TASK
  int core = xPortGetCoreID();
  Serial.print("Main code running on core ");
  Serial.println(core);

  // -- Create the FastLED show task
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);
  #endif
  Serial.println("Display initialized");
}

void loop() {

  loop_display();

}

void loop_display() {
  EVERY_N_MILLISECONDS( 16 ) {
    int cont = gPatterns[state.display_mode]();
    if( update_display || cont > 0 ) {
      FastLED.setBrightness(state.light);
      #ifdef FASTLED_USE_TASK
      FastLEDshowESP32();
      #else
      FastLED.show();
      #endif
      update_display = false;
    }
  }
}

/* -1 = stop me
 *  0 = nothing changed, I may continue
 *  1 = something may have changed, I may continue
 *  2 = something may have changed, I must continue
 */

int sinefield() {
  float step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte y = 0; y < ROWS; y++ ) {
    hue = step + (37 * sin( ((y*step)/(ROWS*PI)) * 0.04 ) );
    for( byte x = 0; x < COLS; x++ ) {
      hue += 17 * sin(x/(COLS*PI));
      LED(x, y) = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
  return 1;
}

int sinefield2() {
  float step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte x = 0; x < COLS; x++ ) {
    hue = step + (37 * sin( ((x*step)/(COLS*PI)) * 0.04 ) );
    for( byte y = 0; y < ROWS; y++ ) {
      hue += 17 * sin(y/(ROWS*PI));
      LED(x, y) = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
  return 1;
}

int test() {
  fadeToBlackBy(leds, NUM_LEDS, 1);
  LED(0,0) = CRGB(255,0,0);
  LED(COLS-1, ROWS-1) = CRGB(0,0,255);
  for( int y = 0; y < ROWS; y++) {
    LED(COLS/3, y) = CRGB(255,0,255);
    LED(2*(COLS/3), y) = CRGB(0,255,0);
  }
  return 1;
}

uint32_t counter = 0;

int test2() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  counter = (counter + 1) % NUM_LEDS;
  leds[counter] = CRGB(255,0,0);
  return 1;
}

int test3() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  counter = ((counter + 1) % COLS);
  for( int y = 0; y < ROWS; y++) {
    LED(counter, y) = CRGB(0,0,255);
  }
  return 1;
}

int juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 25);
  byte dothue = (millis() >> 9) & 0xFF;
  for( int y = 0; y < ROWS; y++ ) {
    for( int i = 0; i < 6; i++) {
      LED(beatsin16( 1+(i*(12*state.ratio))+y, 0, COLS-1 ), y) |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
  return 1;
}

int plain() {
  fill_rainbow(leds, NUM_LEDS, (millis() / (long)(128.0 * (1.0-state.ratio))) & 0xFF, (256/NUM_LEDS)*(0.5+state.ratio));
  return 1;
}

typedef struct Vector {
  double x1;
  double x2;
} Vector;

typedef struct Matrix {
  double a11;
  double a12;
  double a21;
  double a22;
} Matrix;

struct Matrix multiply(struct Matrix m1, struct Matrix m2) {
  Matrix r = {
      .a11 = m1.a11*m2.a11 + m1.a12*m2.a21,
      .a12 = m1.a11*m2.a12 + m1.a12*m2.a22,
      .a21 = m1.a21*m2.a11 + m1.a22*m2.a21,
      .a22 = m1.a21*m2.a12 + m1.a22*m2.a22
    };
  return r;
};

struct Vector multiply(struct Matrix m, struct Vector v) {
  Vector r = {
      .x1 = (m.a11*v.x1) + (m.a12*v.x2),
      .x2 = (m.a21*v.x1) + (m.a22*v.x2)
    };
  return r;
}

struct Vector add(struct Vector v1, struct Vector v2) {
  Vector r = {
    .x1 = v1.x1 + v2.x2,
    .x2 = v1.x2 + v2.x2
  };
  return r;
}

inline double sines(double x, double y) {
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

inline double basefield(double x, double y) {
  return (cos(x) * sin(y) * cos(sqrt((x*x) + (y*y))));
}

inline double addmod(double x, double mod, double delta) {
  x = fmodf(x + delta, mod);
  x += x<0 ? mod : 0;
  return x;
}

inline double addmodpi(double x, double delta) {
  return addmod(x, 2*PI, delta);
}

double fx = 1.0/(COLS/PI);
double fy = 1.0/(ROWS/PI);

double pangle = random(4096) / (4096.0/PI);
double angle = 0;
double sx = random(4096) / (4096.0/PI);
double sy = random(4096) / (4096.0/PI);
double tx = random(4096) / (4096.0/PI);
double ty = random(4096) / (4096.0/PI);
double cx = random(4096) / (4096.0/PI);
double cy = random(4096) / (4096.0/PI);
double rcx = 0;
double rcy = 0;
double basecol = random(4096) / 4096.0;
//const double f = 0.33;

int sinematrix3() {
  EVERY_N_MILLISECONDS( 20 ) {
  double f = state.ratio;
  pangle = addmodpi( pangle, (0.0133 + (angle/256)) * (0.2+f) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.000673 * f );
  sy = addmodpi( sy, 0.000437 * f );
  tx = addmodpi( tx, 0.000539 * f );
  ty = addmodpi( ty, 0.000493 * f );
  cx = addmodpi( cx, 0.000571 * f );
  cy = addmodpi( cy, 0.000679 * f );
  rcx = (COLS/2) + (sin(cx) * COLS);
  rcy = (ROWS/2) + (sin(cy) * ROWS);
  basecol = addmod( basecol, 1.0, 0.00097 * f );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/32.0 + 0.01,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/32.0 + 0.01
  };
  Vector translate = {
    .x1 = sin(tx) * COLS,
    .x2 = sin(ty) * ROWS
  };

  for( int x = 0; x < COLS; x++ ) {
    for( int y = 0; y < ROWS; y++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      LED(x,y) = CHSV((basecol+basefield(c.x1, c.x2))*255, 255, 255);
    }
  }
  return 1;
  }
  return 0;
}


CRGB palette(byte shade) {
  double r = 1-cos(((shade/255.0)*PI)/2);
  double g = ((1-cos(((shade/255.0)*6*PI)/2))/2) * (r*r*0.5);
  double b = (1-cos((_max(shade-128,0)/128.0)*0.5*PI)) * r;
  return CRGB(r*255, g*255 , b*255);
}

#define COOLDOWN 8
uint16_t firecount = 0;
byte buf[ 2 ][ COLS+2 ][ ROWS+4 ];
byte bufi = 0;

int FireLoop() {
  if( firecount % 4 <= 1 ) {
    for( byte x = 0; x < COLS+2; x++ ) {
      buf[bufi][x][ROWS+2] = (byte)0x32 << random(3);
      buf[bufi][x][ROWS+3] = (byte)0x32 << random(4);
    }
  }
  if( firecount % random(8) == 0 ) {
    byte d = random(COLS-1);
    for( byte x = 1; x <=3; x++) {
      for( byte y = 2; y <= 3; y++)  {
        buf[bufi][d+x][ROWS+y] = 255;
      }
    }
  }
  for( byte x = 1; x <= COLS; x++ ) {
    for( int y = 1; y <= ROWS+2; y++ ) {
      int delta = ((buf[bufi][x-1][y]) + (buf[bufi][x+1][y]) + (buf[bufi][x][y-1]) + (buf[bufi][x][y+1])) / 4;
      if( delta <= COOLDOWN ) delta = COOLDOWN;
      buf[bufi^1][x][y-1] = (delta - COOLDOWN);
    }
  }
  bufi ^= 1;
  for( byte x = 0; x < COLS; x++ ) {
    for( byte y = 0; y < ROWS; y++ ) {
      LED(x, y) = palette( buf[bufi][x+1][y+1] );
    }
  }
  firecount++;
  return 1;
}

int perlinmatrix() {
  EVERY_N_MILLISECONDS(20) {
    double f = state.ratio;
    pangle = addmodpi( pangle, (0.0133 + (angle/256)) * (0.2+f) );
    angle = cos(pangle) * PI;
    sx = addmodpi( sx, 0.0000673 );
    sy = addmodpi( sy, 0.0000437 );
    tx = addmodpi( tx, 0.0000239 );
    ty = addmodpi( ty, 0.0000293 );
    cx = addmodpi( cx, 0.0000197 );
    cy = addmodpi( cy, 0.0000227 );
    rcx = (COLS/2) + (sin(cx) * COLS);
    rcy = (ROWS/2) + (sin(cy) * ROWS);
    //basecol = addmod( basecol, 1.0, 0.007 );
    basecol = addmod( basecol, 1.0, 0.0097 * f );

    double z1 = (sin(sx)/16.0) + 0.2;
    double z2 = (cos(sy)/16.0) + 0.2;

    Matrix rotate = {
      .a11 = cos(angle),
      .a12 = -sin(angle),
      .a21 = sin(angle),
      .a22 = cos(angle)
    };
    Matrix zoom = {
      .a11 = z1,
      .a12 = 0, //atan(cos(sx2)),
      .a21 = 0, //atan(cos(sy2)),
      .a22 = z2
    };
    Vector translate = {
      .x1 = sin(tx) * COLS,
      .x2 = sin(ty) * ROWS
    };

    Vector o1 = add(multiply( multiply(rotate, zoom), { .x1 = 0, .x2 = 0 } ), translate);
    Vector o2 = add(multiply( multiply(rotate, zoom), { .x1 = COLS-1, .x2 = ROWS-1 } ), translate);
    //Serial.printf("pangle: %10.7f   angle: %10.7f  sx: %10.7f  sy: %10.7f  tx: %10.7f  ty: %10.7f  cx: %10.7f  cy: %10.7f  rcx: %10.7f  rcy: %10.7f  basecol: %10.7f\n",
    //  pangle, angle, sx, sy, tx, ty, cx, cy, rcx, rcy, basecol);

//    Serial.printf("pangle: %10.7f   angle: %10.7f  x: %10.7f  y: %10.7f  delta: %10.7f  z1: %10.7f  z2: %10.7f  \n",
//      pangle, angle, o1.x1, o1.x2, o2.x1-o1.x1, z1, z2);


    for( int x = 0; x < COLS; x++ ) {
      for( int y = 0; y < ROWS; y++ ) {
        Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x/32.0, .x2 = y/32.0 } ), translate);
        LED(x,y) = CHSV((basecol + pnoise(c.x1, c.x2, 0))*255, 255, 255);
      }
    }
  }
  return 1;
}


inline int safe(int i) {
  if( i < 0 ) return 0;
  if( i >= NUM_LEDS ) return NUM_LEDS-1;
  return i;
}

inline float square(float x) {
  return x*x;
}
