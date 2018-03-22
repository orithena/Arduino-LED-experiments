#define FASTLED_ALLOW_INTERRUPTS 0
#define DEBUG
//#define DEBUG_PIR

#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    D4
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    12

#define MODE_BUTTON_PIN D3
#define BRIGHTNESS          255
#define FRAMES_PER_SECOND   60
#define TIMEOUT 600000

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


#include <Button.h>

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>

CRGB leds[NUM_LEDS];

uint8_t brightness = BRIGHTNESS;
uint32_t lastpacket = 0;

Button modebutton(MODE_BUTTON_PIN);

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
uint32_t last_ntp_packet_received = 0;
uint32_t last_ntp_packet_sent = 0;
boolean waiting_for_ntp_packet = false;

WiFiManager wifiManager;
uint32_t wifi_start = 0;

time_t local_time = 0;
time_t cur = 12*60*60;
int oldsecond = 0;
int oldminute = 0;

uint8_t daytimebrightness(uint8_t h, uint8_t m) {
  uint8_t bright = 24+(200*_max(0,sin(((((h+22)%24)*60)+m)*PI/1440.0)));
  if(m != oldminute) Serial.printf("%02d:%02d Using brightness: %d\n", h, m, bright);
  return bright;
}
void highlight(uint8_t h, uint8_t m, uint8_t s) {
  leds[clock_led(h)].b = _max(255 - (m*4.25), leds[clock_led(h)].b);
  leds[clock_led(h+1)].b = _max((m*4.25), leds[clock_led(h+1)].b);
  leds[clock_led(m/5)].r = _max(255 - ((((m%5)*60) + s)*0.85), leds[clock_led(m/5)].r);
  leds[clock_led((m/5)+1)].r = _max(((((m%5)*60) + s)*0.85), leds[clock_led((m/5)+1)].r);
}
void rainbow_highlight(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(s,64);
  highlight(h,m,s);
}
void rainbow_highlight_var(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(s,32+(daytimebrightness(h,m) * 0.3));
  highlight(h,m,s);
}
void led_multiply(CRGB *led, double f) {
  led->r *= f*f;
  led->g *= f*f;
  led->b *= f*f;
}
void rainbow_darken(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(s, 255);
  double mfak = (((m%5)*60) + s) * 0.003;
  double hfak = m * 0.015;
  uint8_t mled = m/5;
  if( oldsecond != s )
    Serial.printf("%02d:%02d:%02d \t led %d * %1.4f \t led %d * %1.4f \t\t led %d * %1.4f \t led %d * %1.4f\n",
      h, m, s, mled, 0.1+mfak, mled+1, 1.0-mfak, h, 0.1+hfak, h+1, 1.0 - hfak);
  if( oldsecond != s )
    Serial.printf("\t\t led %02d: %02x%02x%02x \t led %02d: %02x%02x%02x \t\t led %02d: %02x%02x%02x \t led %02d: %02x%02x%02x\n",
      mled, leds[clock_led(mled)].r, leds[clock_led(mled)].g, leds[clock_led(mled)].b,
      mled+1, leds[clock_led(mled+1)].r, leds[clock_led(mled+1)].g, leds[clock_led(mled+1)].b,
      h, leds[clock_led(h)].r, leds[clock_led(h)].g, leds[clock_led(h)].b,
      h+1, leds[clock_led(h+1)].r, leds[clock_led(h+1)].g, leds[clock_led(h+1)].b
    );
  led_multiply(&leds[clock_led(mled)], 0.1 + mfak);
  led_multiply(&leds[clock_led(mled + 1)], 1.0 - mfak);
  led_multiply(&leds[clock_led(h)], 0.1 + hfak);
  led_multiply(&leds[clock_led(h+1)], 1.0 - hfak);
  if( oldsecond != s )
    Serial.printf("\t\t led %02d: %02x%02x%02x \t led %02d: %02x%02x%02x \t\t led %02d: %02x%02x%02x \t led %02d: %02x%02x%02x\n",
      mled, leds[clock_led(mled)].r, leds[clock_led(mled)].g, leds[clock_led(mled)].b,
      mled+1, leds[clock_led(mled+1)].r, leds[clock_led(mled+1)].g, leds[clock_led(mled+1)].b,
      h, leds[clock_led(h)].r, leds[clock_led(h)].g, leds[clock_led(h)].b,
      h+1, leds[clock_led(h+1)].r, leds[clock_led(h+1)].g, leds[clock_led(h+1)].b
    );
}
void rainbow_only(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(s, 255);
}
void rainbow_slow_bright(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(m, 255);
}
void rainbow_slow_var(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(m, daytimebrightness(h,m));
}
void solid_seconds_var(uint8_t h, uint8_t m, uint8_t s) {
  fill_solid(leds, NUM_LEDS, CHSV(s*4.25, 255, daytimebrightness(h,m)));
}
void solid_seconds_bright(uint8_t h, uint8_t m, uint8_t s) {
  if( oldsecond != s )
    Serial.printf("%02d:%02d:%02d Hue: %d\n", h,m,s, (uint8_t)(s*4.25));
  fill_solid(leds, NUM_LEDS, CHSV(s*4.25, 255, 255));
}
void rainbow_fast_highlight(uint8_t h, uint8_t m, uint8_t s) {
  rainbow((s*5)%60, 64);
  highlight(h,m,s);
}
void rainbow_fast(uint8_t h, uint8_t m, uint8_t s) {
  rainbow((s*5)%60, daytimebrightness(h,m));
}
void rainbow_only_dark(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(s, 96);
}
void rainbow_only_var(uint8_t h, uint8_t m, uint8_t s) {
  rainbow(s, daytimebrightness(h,m));
}
void rainbow(uint8_t s, int bright) {
  for( int a = 0; a < 12; a++ ) {
    leds[clock_led(a)] = CHSV(((256/60.0)*s) + ((256/12.0)*(11-a)), 255, bright);
  }
}

typedef void (*SimplePatternList[])(uint8_t h, uint8_t m, uint8_t s);
SimplePatternList gPatterns = { 
    rainbow_highlight_var, rainbow_highlight, rainbow_darken, 
    rainbow_only_var, rainbow_only_dark, rainbow_only, 
    rainbow_fast, rainbow_fast_highlight, 
    rainbow_slow_bright, rainbow_slow_var,
    solid_seconds_var, solid_seconds_bright
  };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void setup() {
  delay(200);
  Serial.begin(230400);
  modebutton.begin();
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  fill_solid(leds, NUM_LEDS, CRGB(255,0,0));
  FastLED.show();

  delay(500);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("ClockSetup");

  fill_solid(leds, NUM_LEDS, CRGB(0,0,255));
  FastLED.show();

  delay(500);
}

uint32_t showmodeuntil = 0;

void showmode() {
  fill_solid(leds, NUM_LEDS, CHSV(0, 255, 32) );
  leds[clock_led(gCurrentPatternNumber)] = CRGB(255,255,255);
}

void loop()
{  
  cur = get_current_time(); //NORMAL MODE
  //cur += 5; //FAST TIME DEBUG MODE

  if(modebutton.pressed()) {
    nextPattern();
    Serial.printf("Caught Button, mode is now %d\n", gCurrentPatternNumber);
    showmodeuntil = millis() + 500;
  }

  int h = hour(cur);
  int m = minute(cur);
  int s = second(cur);

  if( millis() < showmodeuntil ) {
    showmode(); 
    FastLED.show();
  } else if( last_ntp_packet_sent != 0 && last_ntp_packet_received == 0 ) {
    fill_solid(leds, NUM_LEDS, CRGB(0,255,0));
    FastLED.show();
  } else if( s != oldsecond || showmodeuntil != 0 ) {
    gPatterns[gCurrentPatternNumber](h, m, s);
    showmodeuntil = 0;
    FastLED.show();
  }
  
  oldsecond = s;
  oldminute = m;
}

void nextPattern()
{
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns );
}

inline int clock_led(byte num) {
  return NUM_LEDS - 1 - (num % 12);
  //return num % 12;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

inline time_t calc_local_time_with_rtc(time_t local_time, uint32_t ms) {
  return local_time + ((ms - last_ntp_packet_received)/1000);
}
time_t get_current_time() {
  uint32_t ms = millis();
  int wifi_status = WiFi.status();
  if (wifi_status != WL_CONNECTED && (ms - wifi_start) > 10000 ) {
    if (WiFi.SSID()) {
      //trying to fix connection in progress hanging
      ETS_UART_INTR_DISABLE();
      WiFi.persistent(false);      
      WiFi.disconnect();          
      WiFi.persistent(true);
      ETS_UART_INTR_ENABLE();

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
    Serial.println("Sending UDP NTP request packet");
    udp.begin(localPort);

    Serial.print("Resolving IP Address of pool.ntp.org... ");
    WiFi.hostByName(ntpServerName, timeServerIP);
    Serial.println(timeServerIP);
    if( !timeServerIP ) {
      ETS_UART_INTR_DISABLE();
      WiFi.persistent(false);      
      WiFi.disconnect();          
      WiFi.persistent(true);
      ETS_UART_INTR_ENABLE();
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
      Serial.print("Receiving packet... ");
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      udp.flush();
      udp.stop();
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;

      if( epoch > 1420066800UL && epoch < 2146863600UL ) {
        TimeChangeRule *tcr;
        time_t utc = epoch;
    
        local_time = CE.toLocal(utc, &tcr);
        waiting_for_ntp_packet = false;
        last_ntp_packet_received = _max(1, ms); // don't let last_ntp_packet_received ever be 0 again except at boot.
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

