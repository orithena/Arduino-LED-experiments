//#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_RMT_BUILTIN_DRIVER true
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#define FASTLED_SHOW_CORE 1

#include <WebServer.h>
#include <ESPUI.h>
#include <NoiselessTouchESP32.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

//#define DEBUG

#define DATA_PIN    18
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER BRG
#define NUM_LEDS    90
CRGB leds[NUM_LEDS];


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

typedef struct State {
  int light;
  float ratio;
  uint8_t display_mode;
  bool display_static;
} State;

State state = {
  light: ui_to_val(50),
  ratio: 0.5,
  display_mode: 0,
  display_static: false
};


#define SHORT_TOUCH 12
#define LIGHTEN_THROTTLE 2

NoiselessTouchESP32 touch(T6, 8, 12);

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef int (*SimplePatternList[])();
SimplePatternList gPatterns = { juggle, plain };

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

void setup() {
  Serial.begin(230400);
  Serial.printf("\n\nKitchenLamp is booting...\n\n");
  SPIFFS.begin();

  load_state();
  setup_led_panels();
  setup_wifi();  
  setup_ui();
  setup_display();

}

void slider_callback(Control sender, int type) {
  int value = atoi(sender.value.c_str());
  Serial.printf("id: %d\tval: %d\tsender.type: %d\ttype: %d\n", sender.id, value, sender.type, type);
  switch( sender.id ) {
    case 0: state.light = ui_to_val(value); break;
    case 1: state.ratio = value / 100.0; break;
  }
  ESPUI.updateSlider(0, val_to_ui(state.light), -1);
  update_lights = true;
}

void save_callback(Control sender, int type) {
  if( type > 0 ) {
    save_state();
    print_state();
  }
}

void pattern_callback(Control sender, int type) {
  if( type > 0 ) {
    nextPattern();
    print_state();
  }
}

void static_callback(Control sender, int type) {
  state.display_static = type > 0;
  update_display = true;
  print_state();
}

void setup_ui() {
  char buf[10];
  sprintf(buf, "%d", val_to_ui(state.light));
  ESPUI.slider("Brightness", &slider_callback, COLOR_EMERALD, buf);
  sprintf(buf, "%d", state.ratio * 100);
  ESPUI.slider("Bottom <--> Top", &slider_callback, COLOR_EMERALD, buf);
  ESPUI.button("Next display pattern", &pattern_callback, COLOR_SUNFLOWER);
  ESPUI.switcher("Auto rotate display modes", !(state.display_static), &static_callback, COLOR_SUNFLOWER);
  ESPUI.button("Save state as default", &save_callback, COLOR_ALIZARIN);
  ESPUI.begin("Kitchen Light Control");
}

void setup_wifi() {
  WiFiManager wm;
  wm.setHostname("kitchenlight");
  if( wm.autoConnect("KitchenLightSetup") ) {
    Serial.print("WiFi set up with IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error connecting to WiFi");
  }
}

void setup_display() {
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);

  int core = xPortGetCoreID();
  Serial.print("Main code running on core ");
  Serial.println(core);

  // -- Create the FastLED show task
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);
}

void setup_led_panels() {
  ledcSetup(topChannel, freq, resolution);
  ledcSetup(bottomChannel, freq, resolution);

  ledcAttachPin(16, topChannel);
  ledcAttachPin(17, topChannel);
  ledcAttachPin(21, topChannel);

  ledcAttachPin(22, bottomChannel);
  ledcAttachPin(23, bottomChannel);
  ledcAttachPin(19, bottomChannel);

  update_lights = true;
  brightness_control();
}

int touch_count = 0;

void loop() {

  loop_touch_control();
  brightness_control();
  loop_display();

}

void loop_display() {
  EVERY_N_MILLISECONDS( 10 ) {
    int cont = gPatterns[state.display_mode]();
    if( update_display || cont > 0 ) {
      //FastLEDshowESP32();
      FastLED.show();
      update_display = false;
    }
    switch(cont) {
      case -1: 
        nextPattern(); 
        break;
      case  0:
      case  1:
        EVERY_N_SECONDS( 60 ) {
          if( !state.display_static ) {
            nextPattern();
          }
        }
        break;
      case  2:
        break;
    }
  }
}

/* -1 = stop me
 *  0 = nothing changed, I may continue
 *  1 = something may have changed, I may continue
 *  2 = something may have changed, I must continue
 */

int juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 3);
  byte dothue = 0;
  for( int l = 0; l < 6; l++) {
    for( int i = 0; i < 3; i++) {
      leds[safe((l*15)+beatsin16( 1+(i*6)+l, 0, 15 ))] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
  return 1;
}

int plain() {
  fill_rainbow(leds, NUM_LEDS, 0, 13);
  return 0;
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  state.display_mode = (state.display_mode + 1) % ARRAY_SIZE( gPatterns );
  update_display = true;
}

void brightness_control() {
  if( update_lights ) {
    float top_light = (2.0 - (state.ratio > 0.5 ? (state.ratio-0.5)*2.0 : 0)) * (state.ratio);
    float bottom_light = (2.0 - (state.ratio < 0.5 ? (0.5-state.ratio)*2.0 : 0)) * (1.0 - state.ratio);
    int t = _min(255, state.light * top_light);
    int b = _min(255, state.light * bottom_light);
    Serial.printf("Light update: %3d. Ratio: %5.3f => %5.3f/%3d top, %5.3f/%3d bottom, \n", state.light, state.ratio, top_light, t, bottom_light, b); 
    ledcWrite(topChannel, t);
    ledcWrite(bottomChannel, b);
    update_lights = false;
  }
}

void loop_touch_control() {
  EVERY_N_MILLISECONDS( 35 ) {
    bool touching = touch.touching();
    #ifdef DEBUG
    Serial.printf("Touching: %2d\tLight: %3d\ttouch_count: %3d\thyst_value: %3d\tmean_value:%3d\n", touching, state.light, touch_count, touch.last_value(), touch.value_from_history());
    #endif
    if( !touching && touch_count > 0 && touch_count < SHORT_TOUCH ) {
      state.light = _max(0, state.light / 2);
      update_lights = true;
      ESPUI.updateSlider(0, val_to_ui(state.light), -1);
    }
    touch_count = touching ? touch_count + 1 : 0;
    if( touching && touch_count >= SHORT_TOUCH && touch_count % LIGHTEN_THROTTLE == 0 ) {
      state.light = _min(255, state.light + 1 + (state.light / 64));
      update_lights = true;
      ESPUI.updateSlider(0, val_to_ui(state.light), -1);
    }
  }
}

inline void load_state() {
  print_state();
  File f = SPIFFS.open("/state.data", "r");
  
  if (!f)
  {
    Serial.println(F("Failed to open state.data")); 
  } else {
    Serial.println(F("Opened state.data"));
  
    char * data = reinterpret_cast<char*>(&state);
    size_t bytes = f.readBytes(data, sizeof(state));
    Serial.printf("Read %d bytes from SPIFFS\n", bytes);
    f.close();
    update_lights = true;
    update_display = true;
    print_state();
  }
}

inline void print_state() {
  Serial.printf("State:\tlight=%d\tratio=%6.3f\tmode=%d\tstatic=%d\n", state.light, state.ratio, state.display_mode, state.display_static);
}

inline void save_state() {
  File f = SPIFFS.open("/state.data", "w+");
  if (!f)
  {
   Serial.println(F("Failed to open state.data"));            
  } else {
    Serial.println(F("Opened state.data for UPDATE...."));
  
    const uint8_t * data = reinterpret_cast<const uint8_t*>(&state);
    size_t bytes = f.write( data, sizeof(state) );
  
    Serial.printf("Wrote %d bytes to SPIFFS\n", bytes);
    f.close();
  }
}

inline int safe(int i) {
  if( i < 0 ) return 0;
  if( i >= NUM_LEDS ) return NUM_LEDS-1;
  return i;
}

inline float square(float x) {
  return x*x;
}

