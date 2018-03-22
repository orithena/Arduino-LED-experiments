/*** FastLED stuff ***/

#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif


/*** LED Control data ***/

#define DATA_PIN    D3
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
const uint8_t kMatrixHeight = 14; // Matrix Height
const uint8_t kMatrixWidth = 14;  // Matrix Width
#define NUM_LEDS    196           // Height * Width
CRGB leds[NUM_LEDS];

/*** Base data ***/

#define MIN_BRIGHTNESS         16
#define MAX_BRIGHTNESS         255
#define FRAMES_PER_SECOND  60
//#define DEBUG

/*** Timezone definitions ***/

#include <TimeLib.h>
#include <Timezone.h>

//Central European Time (Berlin, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone TZ(CEST, CET);

/*** additional stuff ***/
extern "C" {
#include "user_interface.h"
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

/*** WiFi Manager stuff ***/

#include "ESP8266WiFi.h"

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

WiFiManager wifiManager;
uint32_t wifi_start = 0;        // when did we start WiFi? (used for reconnection timeouts)

/*** Time retrieval variables and objects ***/

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

uint32_t last_ntp_packet_received = 0;  // when did we last receive a NTP response?
uint32_t last_ntp_packet_sent = 0;      // when did we last send a NTP request?
boolean waiting_for_ntp_packet = false;

time_t local_time = 0;
int oldsecond = 0;                      // used for serial output minimization
int oldminute = 0;                      // used for serial output minimization


/*** Setup ***/

void setup() {
  Serial.begin(230400);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("MatrixDemoSetup");
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control (will be overwritten as soon as a time server did answer)
  FastLED.setBrightness(MIN_BRIGHTNESS);
}

/*** Pattern selection ***/

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { sinefield, sinematrix, sinematrix2, sinematrix3 };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
#define PATTERN_SECONDS 60        // how long a pattern is shown in seconds

/*** central loop ***/
  
void loop()
{
  time_t cur = get_current_time();
    
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  FastLED.setBrightness(daytimebrightness(hour(cur), minute(cur))); // set brightness according to time
  
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  EVERY_N_SECONDS( PATTERN_SECONDS ) { nextPattern(); } // change patterns periodically
}

/*** Pattern control functions ***/

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

/*** Brightness Control function ***/

uint8_t daytimebrightness(uint8_t h, uint8_t m) {
  uint8_t bright = MIN_BRIGHTNESS+((MAX_BRIGHTNESS-MIN_BRIGHTNESS)*_max(0,sin(((((h+22)%24)*60)+m)*PI/1440.0)));
  if(m != oldminute) Serial.printf("%02d:%02d Using brightness: %d\n", h, m, bright);
  oldminute = m;
  return bright;
}

/*** Linear Algebra math functions needed for projection ***/

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

/*** Base field functions ***/

inline double sines3D(double x, double y) {
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

inline double sinecircle3D(double x, double y) {
  return (cos(x) * sin(y) * cos(sqrt((x*x) + (y*y))));
}

/*** math helper functions ***/

inline double addmod(double x, double mod, double delta) {
  x = x + delta;
  while( x >= mod ) x -= mod;
  while( x <  0.0 ) x += mod;
  return x;
}

inline double addmodpi(double x, double delta) {
  return addmod(x, 2*PI, delta);
}

/*** pattern run variables, used by all sinematrix patterns ***/

// primary matrix coefficient run variables
double pangle = 0;      // "proto-angle"
double angle = 0;       // rotation angle
double sx = 0;          // scale factor x
double sy = 0;          // scale factor y
double tx = 0;          // translation x
double ty = 0;          // translation y
double cx = 0;          // center offset x (used for rcx)
double cy = 0;          // center offset y (used for rcy)
double rcx = 0;         // rotation center offset x
double rcy = 0;         // rotation center offset y

// secondary set of matrix coefficient run variables (used for "shadow overlay" in sinematrix2)
double angle2 = 0;      // angle 
double sx2 = 0;         // scale factor x
double sy2 = 0;         // scale factor y
double tx2 = 0;         // translation x
double ty2 = 0;         // translation y

double basecol = 0;     // base color offset to start from for each frame

/*** sinematrix pattern functions ***
 *  
 * Each of these functions are structured the same:
 *  1. increment run variables by a small amount, each by a different amount, modulo their repetition value.
 *      Do play around with the increments, I just recommend using fractions of different prime numbers
 *      to avoid pattern repetition. Increasing the increments normally speeds up the animation.
 *      (In case they are fed into sin/cos, the repetition value would be PI. If they go 
 *      into hue, it would be 1.0 or 256, depending on the implementation)
 *  2. construct matrices using the run variables as coefficients.
 *  3. For each pixel, interpret their x and y index as a Vector and multiply that with the 
 *      constructed matrices, and add the translation vectors. 
 *      Using a different operation order, you may get different results!
 *      
 * In summary, you may think of these pattern functions as a camera arm that's mounted in the origin
 * of a static curve in 3-dimensional space (which is defined by the functions sines3D and sinecircle3D),
 * where the z axis is mapped to the hue in HSV color space. Then, by changing the coefficients of the 
 * matrices, that camera arm moves the camera seemingly randomly around, pointing it to different areas 
 * of the pattern while the camera zooms in and out and rotates... 
 * 
 * Feel free to insert more alternating rotation matrices and translation vectors ^^
 */

void sinematrix() {
  angle = addmodpi( angle, 0.019 );
  sx = addmodpi( sx, 0.017);
  sy = addmodpi( sy, 0.013);
  tx = addmodpi( tx, 0.023 );
  ty = addmodpi( ty, 0.029 );
  basecol = addmod( basecol, 1.0, 0.01 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix scale = {
    .a11 = sin(sx)/2.0,
    .a12 = 0,
    .a21 = 0,
    .a22 = sin(sy)/2.0
  };
  Vector translate = {
    .x1 = sin(tx),
    .x2 = sin(ty)
  };
  
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(scale, rotate), { .x1 = x-7, .x2 = y-7 } ), translate);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines3D(c.x1, c.x2))*255, 255, 255);
    }
  }
} 

void sinematrix2() {
  pangle = addmodpi( pangle, 0.0133 + (angle/256) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00173 );
  sy = addmodpi( sy, 0.00137 );
  tx = addmodpi( tx, 0.00239 );
  ty = addmodpi( ty, 0.00293 );
  cx = addmodpi( cx, 0.00197 );
  cy = addmodpi( cy, 0.00227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  angle2 = addmodpi( angle2, 0.0029 );
  sx2 = addmodpi( sx2, 0.0041);
  sy2 = addmodpi( sy2, 0.0031);
  tx2 = addmodpi( tx2, 0.0011 );
  ty2 = addmodpi( ty2, 0.0023 );
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix scale = {
    .a11 = sin(sx)/4.0 + 0.25,
    .a12 = 0,
    .a21 = 0,
    .a22 = cos(sy)/4.0 + 0.25
  };
  Vector translate = {
    .x1 = (-kMatrixWidth/2) + (sin(tx) * kMatrixWidth) + rcx,
    .x2 = (-kMatrixHeight/2) + (sin(ty) * kMatrixHeight) + rcy
  };

  Matrix rotate2 = {
    .a11 = cos(angle2),
    .a12 = -sin(angle2),
    .a21 = sin(angle2),
    .a22 = cos(angle2)
  };
  Matrix scale2 = {
    .a11 = sin(sx2)/2.0,
    .a12 = 0,
    .a21 = 0,
    .a22 = sin(sy2)/2.0
  };
  Vector translate2 = {
    .x1 = sin(tx2),
    .x2 = sin(ty2)
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, scale), { .x1 = x, .x2 = y } ), translate);
      Vector c2 = add(multiply( multiply(scale2, rotate2), { .x1 = x, .x2 = y } ), translate2);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines3D(c.x1, c.x2))*255, 255, 31+(sines3D(c2.x1-10, c2.x2-10)*224));
    }
  }
}

void sinematrix3() {
  pangle = addmodpi( pangle, 0.0133 + (angle/256) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
  tx = addmodpi( tx, 0.00239 );
  ty = addmodpi( ty, 0.00293 );
  cx = addmodpi( cx, 0.00197 );
  cy = addmodpi( cy, 0.00227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  angle2 = addmodpi( angle2, 0.0029 );
  sx2 = addmodpi( sx2, 0.0041);
  sy2 = addmodpi( sy2, 0.0031);
  tx2 = addmodpi( tx2, 0.0011 );
  ty2 = addmodpi( ty2, 0.0023 );
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix scale = {
    .a11 = sin(sx)/4.0 + 0.15,
    .a12 = 0,
    .a21 = 0,
    .a22 = cos(sy)/4.0 + 0.15
  };
  Vector translate = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, scale), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sinecircle3D(c.x1, c.x2))*255, 255, 255);
    }
  }
}


/*** sinefield demo pattern ***
 *  
 *  I don't know how this pattern works. I just experimented and it looked nice.
 */

void sinefield() {
  double step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    hue = step + (37 * sin( ((y*step)/(kMatrixHeight*PI)) * 0.04 ) );
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      hue += 17 * sin(x/(kMatrixWidth*PI));
      leds[ (x*kMatrixHeight) + y ] = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
}

/*** WiFi helper functions ***/

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

inline void disconnectWiFi() {
  ETS_UART_INTR_DISABLE();
  WiFi.persistent(false);   // needed to avoid deleting WiFi credentials on disconnect()
  WiFi.disconnect();          
  WiFi.persistent(true);
  ETS_UART_INTR_ENABLE();
}

/*** Time management functions ***/

inline time_t calc_local_time_with_rtc(time_t local_time, uint32_t ms) {
  return local_time + ((ms - last_ntp_packet_received)/1000);
}

time_t get_current_time() {
  uint32_t ms = millis();
  int wifi_status = WiFi.status();
  if (wifi_status != WL_CONNECTED && (ms - wifi_start) > 10000 ) {
    if (WiFi.SSID()) {
      // trying to fix connection in progress hanging
      disconnectWiFi();
      WiFi.begin();
    }
    wifi_start = ms;
  } else if (wifi_status != WL_CONNECTED) {
    Serial.print(".");
  } else if( wifi_status == WL_CONNECTED &&
            ( last_ntp_packet_sent == 0 
              || (waiting_for_ntp_packet && ((ms - last_ntp_packet_sent) > 10000) ) 
              || (!waiting_for_ntp_packet && (ms - last_ntp_packet_received) > 3600000 && hour(calc_local_time_with_rtc(local_time, ms)) < 5 )
            )) {
    // send NTP request if WiFi is connected and:
    //    a) we never sent an NTP request
    //    b) or we're waiting for more than 10 seconds on a response
    //    c) or the last sync was more than 1 hour ago and it's currently between 0:00 and 5:00 (i.e. re-sync time only at night!)
    Serial.println("Starting UDP to send an NTP request.");
    udp.begin(localPort);

    Serial.print("Resolving IP Address... ");
    WiFi.hostByName(ntpServerName, timeServerIP);
    Serial.println(timeServerIP);
    if( !timeServerIP ) {
      disconnectWiFi();
    } else {
      Serial.print("Sending packet... ");
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  
      waiting_for_ntp_packet = true;
      last_ntp_packet_sent = ms;
      Serial.println("continue");
    }
  } else if(wifi_status == WL_CONNECTED) {
    int cb = udp.parsePacket();
    if (cb >= NTP_PACKET_SIZE) {
      // hey, we've got a response via UDP! Maybe it's an NTP packet?
      Serial.print("Receiving packet... ");
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      udp.flush();
      udp.stop();
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;

      if( epoch > 1420066800UL && epoch < 2146863600UL ) { // sanity check to see if the packet contains valid NTP data.
        TimeChangeRule *tcr;
        time_t utc = epoch;
    
        local_time = TZ.toLocal(utc, &tcr);
        waiting_for_ntp_packet = false;
        last_ntp_packet_received = ms;

        Serial.printf("Got packet: %02d:%02d:%02d\n", hour(local_time), minute(local_time), second(local_time));
      }
    }
  }
  return calc_local_time_with_rtc(local_time, ms);
}

unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


