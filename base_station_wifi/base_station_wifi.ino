#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define DEBUG
//#define DEEP_DEBUG

const char* firmware_type = "QM_WIFI_AP_BASE";
const char* firmware_version = "0.99.1";
const char* firmware_timestamp = __TIMESTAMP__;
const char* wifi_ssid = "IoT-1D1071C5";
const char* wifi_psk = "okngouiesesinempebusac";

#define RESET_AT_MS 600000

const unsigned int listenport = 31313;
IPAddress ip(10, 0, 0, 1);
IPAddress gateway(0,0,0,0);
IPAddress subnet(255, 255, 0, 0);

#define DATA_SIZE 250

#define MAX_CONCURRENT_CLIENTS 4
WiFiClient tcpclients[MAX_CONCURRENT_CLIENTS];

typedef struct Packet {
  uint32_t number;
  char data[DATA_SIZE];
  char nul = 0;
} Packet;

Packet read_buffer;
uint32_t read_counter;

#define RINGBUFFER_SIZE 8

Packet ringbuffer[RINGBUFFER_SIZE];

uint32_t ringbuffer_writer = 0;
uint32_t ringbuffer_reader = 0;

uint32_t packet_counter = 0;

WiFiServer server(listenport);

void setup() {
  init_serial();
  print_hello();

  init_wifi(&server);
}

void loop() {
  receive_wifi_data();
  send_serial_data();
  if( millis() > RESET_AT_MS ) {
    ESP.restart();
  }
}

void receive_wifi_data() {
  // read data from wifi
  for (int client_num = 0; client_num < MAX_CONCURRENT_CLIENTS; client_num++) {
    if( tcpclients[client_num] ) {
      if( tcpclients[client_num].status() == CLOSED ) {
        tcpclients[client_num].stop();
        #ifdef DEEP_DEBUG
        Serial.print("Stopped client #");
        Serial.println(client_num);
        #endif
      }
      while ( tcpclients[client_num].available() > 0 ) {
        // read the bytes incoming from the client:
        char in = tcpclients[client_num].read();
        // filter allowed characters
        if( 
          (in == 0x20) ||                 // space
          (in == '\t') ||                 // tab
          (in >= 0x22 && in <= 0x23) ||   // " #
          (in >= 0x27 && in <= 0x29) ||   // ' ( )
          (in >= 0x2C && in <= 0x2E) ||   // , - .
          (in >= 0x30 && in <= 0x3B) ||   // 0-9 : ;
          (in == 0x3D) ||                 // =
          (in >= 0x41 && in <= 0x5D) ||   // A-Z [ \ ]
          (in >= 0x61 && in <= 0x7D)      // a-z { | }
        ) {
          if( read_counter < DATA_SIZE ) {
            read_buffer.data[read_counter++] = in;
          }
        }
        else if( in == 0x0A || in == 0x0D ) {
          if( read_counter < 3 ) {
            #ifdef DEEP_DEBUG
            Serial.print("Got 0x");
            Serial.print(in, HEX);
            Serial.print(". Ditching empty packet of size ");
            Serial.print(read_counter);
            Serial.print(": ");
            Serial.println(read_buffer.data);
            #endif
            // no good data comes out of packets that are too small.
            memset(&read_buffer, 0, sizeof(Packet));
            read_counter = 0;
          } else {
            #ifdef DEEP_DEBUG
            Serial.printf("Stations connected to soft-AP = %d\n", WiFi.softAPgetStationNum());
            Serial.printf("%d.%03d", millis() / 1000, millis() % 1000);
            Serial.print(" Got packet for ringbuffer #");
            Serial.println(ringbuffer_writer);
            #endif
            read_buffer.number = packet_counter++;
            // copy received packet to ringbuffer
            memcpy(&(ringbuffer[ringbuffer_writer]), &read_buffer, sizeof(Packet));
            // increase ringbuffer write counter
            ringbuffer_writer = (ringbuffer_writer+1) % RINGBUFFER_SIZE;
            // check for buffer overflow
            if( ringbuffer_writer == ringbuffer_reader ) {
              // buffer overflow -> forget old packets.
              ringbuffer_reader = (ringbuffer_reader+1) % RINGBUFFER_SIZE;
            }
            // zero out next buffer
            memset(&(ringbuffer[ringbuffer_writer]), 0, sizeof(Packet));
            memset(&read_buffer, 0, sizeof(Packet));
            // ready for next packet
            read_counter = 0;
          }
        }
        yield();
      }
      tcpclients[client_num].flush();
    } else {
      tcpclients[client_num] = server.available();
    }
  }
}

void send_serial_data() {
  while( ringbuffer_reader != ringbuffer_writer ) {
    #ifdef DEEP_DEBUG
    Serial.print("Reading packet from ringbuffer #");
    Serial.println(ringbuffer_reader);
    #endif
    #ifdef DEBUG
    Serial.printf("%d.%03d] ", millis() / 1000, millis() % 1000);
    Serial.println(ringbuffer[ringbuffer_reader].data);
    #endif
    Serial1.println(ringbuffer[ringbuffer_reader].data);
    ringbuffer_reader = (ringbuffer_reader+1) % RINGBUFFER_SIZE;
  }
}

void init_wifi(WiFiServer *server) {
  WiFi.mode(WIFI_AP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(wifi_ssid, wifi_psk);
  WiFi.printDiag(Serial1);
  Serial1.print("Created WPA2-PSK protected WiFi with SSID ");
  Serial1.println(wifi_ssid);

  Serial1.print("Listening on TCP port ");
  Serial1.print(listenport);
  Serial1.print(" on interface ");
  IPAddress myAddress = WiFi.softAPIP();
  Serial1.println(myAddress);
  server->begin();
}

void print_hello() {
  Serial1.print("This is firmware '");
  Serial1.print(firmware_type);
  Serial1.print("' version ");
  Serial1.print(firmware_version);
  Serial1.print(" built ");
  Serial1.println(firmware_timestamp);
  #ifdef DEBUG
  Serial.print("This is firmware '");
  Serial.print(firmware_type);
  Serial.print("' version ");
  Serial.print(firmware_version);
  Serial.print(" built ");
  Serial.println(firmware_timestamp);
  #endif
}

void init_serial() {
  Serial1.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial1.println();
  #ifdef DEBUG
  Serial.begin(78400);
  Serial.println();
  #endif
}

//void pause(uint32_t ms) {
//  for(int a = 0; a < ms; a++ ) { delay(1); yield(); }
//}
void pause(uint32_t ms) {
  uint32_t return_at = millis() + ms;
  while(millis() < return_at) { yield(); }
}

