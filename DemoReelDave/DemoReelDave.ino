#define FASTLED_ALLOW_INTERRUPTS 0
//#define DEBUG
//#define DEBUG_PIR
//#define DEBUG_SLEEP

#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    147
#define ADD_LEDS    12
CRGB leds[NUM_LEDS+ADD_LEDS];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND   100
#define BALLS 12
#define TIMEOUT 600000
#define ACTIVITY_THRESHOLD 26

#ifdef DEBUG_SLEEP
#define TIMEOUT 6000
#endif

#define A_PROBE A0

// List of patterns to cycle through.  Each is defined as a separate function below.
uint32_t last_act = 0;
int32_t p_min, p_max, p_scale;
int32_t l_min, l_max;
int32_t s_min, s_max;
int32_t a_probe, a_delta, a_delta_abs, old_probe = 0, old_delta = 0, old_delta_abs = 0;
uint32_t act_range = 0, act_short = 0, act_long = 0, act_range_cap = 0, act_short_cap = 0, act_long_cap = 0;
double a_smooth = 0.0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t brightness = BRIGHTNESS;
boolean inactive = false;

double balls[BALLS][2];
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>

char ssid[] = "WhereIsTheWire";  //  your network SSID (name)
char pass[] = "WirSindHelden";       // your network password

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

uint32_t last_ntp_packet_received = 0;
uint32_t last_ntp_packet_sent = 0;
boolean waiting_for_ntp_packet = false;
time_t local_time = 0;


void setup() {
  Serial.begin(230400);
  WiFi.begin(ssid, pass);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS+ADD_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  gHue = 0;
  gCurrentPatternNumber = 0;

  init_balls();

  pinMode(A_PROBE, INPUT);
  p_min = l_min = s_min = p_max = l_max = s_max = a_probe = analogRead(A_PROBE);
  a_smooth = a_probe;
  last_act = millis();

}

inline int clock_led(byte num) {
//  return NUM_LEDS + ADD_LEDS - 1 - (num % 12);
  return NUM_LEDS + (num % 12);
}

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { sinerainbow, juggle, sinelon, confetti, rainbow, pingpong };

uint32_t lastpacket = 0;

void loop()
{  
  read_PIR();
  check_inactivity();
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  time_t cur = get_current_time();

  int s = second(cur);
  for( int a = 0; a < 12; a++ ) {
    leds[clock_led(a)] = CHSV(((256/60.0)*s) + ((256/12.0)*(11-a)), 255, 96);
  }
  leds[clock_led(hour(cur))].r = 255;
  leds[clock_led(minute(cur)/5)].g = 255;

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 80 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 60 ) { nextPattern(); } // change patterns periodically
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
  init_balls();
}

void check_inactivity() {
  uint32_t ms = millis();
  if( act_long > ACTIVITY_THRESHOLD ) {
    last_act = ms;
  }
  #ifdef DEBUG_SLEEP
  Serial.print("act_long: "); Serial.print(act_long);
  Serial.print("\tms>last_act: "); Serial.print(ms); Serial.print((ms > last_act) ? ">" : "="); Serial.print(last_act);
  Serial.print("\tms_fade:");
  Serial.print(ms-TIMEOUT-last_act);
  Serial.print("\tms_fade/128:");
  Serial.print((ms-TIMEOUT-last_act)/128);
  #endif
  if( last_act+TIMEOUT < ms ) {
    brightness = _max(32, BRIGHTNESS - ((int32_t)(ms-TIMEOUT-last_act)/128));
    #ifdef DEBUG_SLEEP
    Serial.print("\tbright_down:"); Serial.println(brightness);
    #endif
    FastLED.setBrightness(brightness);
    inactive = true;
  } else {
    brightness = _min(BRIGHTNESS, brightness + 4);
    #ifdef DEBUG_SLEEP
    Serial.print("\tbright_up:"); Serial.println(brightness);
    #endif
    FastLED.setBrightness( brightness );
    inactive = false;
  }
}

#define FADE ((32 * ((double)act_range_cap/p_max)))
#define SPEED (0.03125+((double)act_short_cap/l_max))

void read_PIR() {
  a_probe = _max(1, analogRead(A_PROBE));
  a_delta = a_probe - old_probe;
  a_smooth += (a_probe - a_smooth) * 0.125;
  a_delta_abs = abs(a_delta);
  EVERY_N_MILLISECONDS( 75 ) { p_max--; p_min++; }
  EVERY_N_MILLISECONDS( 250 ) { l_max--; l_min++; }
  EVERY_N_MILLISECONDS( 10 ) { s_max--; s_min++; }
  p_max = _max(a_probe, p_max);
  p_min = _min(a_probe, p_min);
  l_max = _max(a_probe, l_max);
  l_min = _min(a_probe, l_min);
  s_max = _max(a_probe, s_max);
  s_min = _min(a_probe, s_min);
  act_range = p_max-p_min;
  act_long  = l_max-l_min;
  act_short = s_max-s_min;
  act_range_cap = _max(0, _min(act_range, 512));
  act_long_cap  = _max(0, _min(act_long,  1024));
  act_short_cap = _max(0, _min(act_short, 256));

#ifdef DEBUG_PIR
  p_scale = 1;
  while( (act_range/p_scale) > 60 ) p_scale++;
  Serial.print("A:"); Serial.print(a_probe);
  Serial.print("\tr:"); Serial.print(p_max); Serial.print("-"); Serial.print(p_min); Serial.print("="); Serial.print(act_range);
  Serial.print("\ts:"); Serial.print(s_max); Serial.print("-"); Serial.print(s_min); Serial.print("="); Serial.print(act_short);
  Serial.print("\tl:"); Serial.print(l_max); Serial.print("-"); Serial.print(l_min); Serial.print("="); Serial.print(act_long);
  Serial.print("\tfade:"); Serial.print(FADE);
  Serial.print("\tspeed:"); Serial.print(SPEED);
  Serial.print("\t");
  printbar((p_max-p_min)/p_scale, (a_probe-p_min)/p_scale, '#', '-');
#endif
  old_probe = a_probe;
  old_delta = a_delta;
  old_delta_abs = a_delta_abs;
}

void sinerainbow() {
  uint32_t ms = millis();
  double f = sin16(ms) / 2048.0;
  for( int a = 0; a < NUM_LEDS; a++ ) {
    leds[NUM_LEDS - 1 - a] = CHSV(sin8((ms >> 4) + (a*f)), 255, 255);
  }
}

void init_balls() {
  for( uint8_t ball = 0; ball < BALLS; ball++) {
    balls[ball][0] = random(NUM_LEDS);
    balls[ball][1] = (random(NUM_LEDS) / 50.0) - (NUM_LEDS/100.0);
  }
}

void pingpong() 
{
  fadeToBlackBy( leds, NUM_LEDS, FADE);
  for( uint8_t ball = 0; ball < BALLS; ball++ ) {
    balls[ball][0] = balls[ball][0] + (balls[ball][1] * SPEED);
    if( (balls[ball][0] < 0) || (balls[ball][0] > NUM_LEDS) ) {
      balls[ball][1] = balls[ball][1] * -1.0;
    }
    paintball(balls[ball][0], 3, (uint8_t)(gHue + act_long_cap + (256/BALLS)*ball));
  }
#ifdef DEBUG
  Serial.println();
#endif
}



void paintball(double pos, uint8_t width, uint8_t hue) 
{
  for( int i = pos-width; i <= pos+width; i++ ) {
    if( i >= 0 && i < NUM_LEDS ) {
      uint8_t val = _max(0, cos(((((double)i)-pos)*(PI*0.5)) / ((double)width)) * 255);
#ifdef DEBUG
      Serial.print(i);
      Serial.print("\t");
      Serial.print(val);
      Serial.print("\t");
#endif
      CRGB new_val = CHSV(hue, 255, val);
      CRGB old_val = leds[i];
      for( uint8_t c = 0; c < 3; c++ ) {
        if( new_val[c] > old_val[c] ) {
          leds[i][c] = new_val[c];
        }
#ifdef DEBUG
        Serial.print(new_val[c]>>4, HEX);
        Serial.print(new_val[c]&0x0F, HEX);
        Serial.print(" ");
#endif
      }
#ifdef DEBUG
      Serial.print("  ");
#endif
    }
  }
#ifdef DEBUG
  Serial.println();
#endif
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue+(2*a_smooth), 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(FADE*3);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 1+(0.5*FADE));
  for( int i = 0; i < (FADE-5+random8(6)); i++ ) {
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( gHue + random8(64) + a_probe, 255, 255);
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  EVERY_N_MILLISECONDS(50) { fadeToBlackBy( leds, NUM_LEDS, 1); }
  int pos = beatsin16(5,0,NUM_LEDS-1);
  leds[pos] = CHSV( gHue+a_probe, 255, 255);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 17;
  CRGBPalette16 palette = RainbowColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, beat-gHue+(i*2)+a_smooth, gHue+(i*10));
  }
}

void juggle() {
  // four colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 4+(0.5*FADE));
  byte dothue = gHue;
  for( int i = 0; i < 4; i++) {
    paintball(beatsin16(i+1,0,NUM_LEDS), 2, dothue);
    dothue += 32+(64*SPEED);
  }
}

#if defined(DEBUG) || defined(DEBUG_PIR) || defined(DEBUG_SLEEP)
void printbar(int n, int t, char c1, char c2) {
  for( int i = 0; i < n; i++ ) {
    if( i <= t ) {
      Serial.print(c1);
    } else {
      Serial.print(c2);
    }
  }
  Serial.println();
}
#endif

uint32_t wifi_start = 0;

time_t get_current_time() {
  uint32_t ms = millis();
  if( last_ntp_packet_received == 0 || inactive ) {
    if (WiFi.status() != WL_CONNECTED && (ms - wifi_start) > 10000 ) {
      WiFi.begin(ssid, pass);
      wifi_start = ms;
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
    } else if ( last_ntp_packet_sent == 0 
                || (waiting_for_ntp_packet && ((ms - last_ntp_packet_sent) > 10000) ) 
                || (!waiting_for_ntp_packet && (ms - last_ntp_packet_received) > 3600000 )
              ) {
      Serial.println("Starting UDP");
      udp.begin(localPort);
  
      Serial.print("Resolving IP Address... ");
      WiFi.hostByName(ntpServerName, timeServerIP);
    
      Serial.print("Sending packet... ");
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  
      waiting_for_ntp_packet = true;
      last_ntp_packet_sent = ms;
      Serial.println("continue");
    } else {
      int cb = udp.parsePacket();
      if (cb > 44) {
        Serial.print("Receiving packet... ");
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        udp.flush();
        udp.stop();
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;
    
        TimeChangeRule *tcr;
        time_t utc = epoch;
    
        local_time = CE.toLocal(utc, &tcr);
        waiting_for_ntp_packet = false;
        last_ntp_packet_received = ms;
        Serial.printf("Got packet: %d:%d:%d\n", hour(local_time), minute(local_time), second(local_time));
      }
    }
  }
  return local_time + ((ms - last_ntp_packet_received)/1000);
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

