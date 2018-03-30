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
#define FRAMES_PER_SECOND  100
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
SimplePatternList gPatterns = { sinefield, sinematrix, sinematrix2, sinematrix3, sinematrix_rgb, affine_fields, gol_loop };
//SimplePatternList gPatterns = { affine_fields };

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


