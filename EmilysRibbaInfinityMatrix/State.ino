
inline void load_state() {
  print_state();
  File f = LittleFS.open("/state.data", "r");
  
  if (!f)
  {
    Serial.println(F("Failed to open state.data")); 
  } else {
    Serial.println(F("Opened state.data"));
  
    char * data = reinterpret_cast<char*>(&state);
    size_t bytes = f.readBytes(data, sizeof(state));
    Serial.printf("Read %d bytes from LittleFS\n", bytes);
    f.close();
    print_state();
  }
}

inline void print_state() {
  Serial.printf("State:\tlight=%d\tmode=%d\tstatic=%d\n", state.light, state.display_mode, state.display_static);
}

inline void save_state() {
  File f = LittleFS.open("/state.data", "w+");
  if (!f)
  {
   Serial.println(F("Failed to open state.data"));            
  } else {
    Serial.println(F("Opened state.data for UPDATE...."));
  
    const uint8_t * data = reinterpret_cast<const uint8_t*>(&state);
    size_t bytes = f.write( data, sizeof(state) );
  
    Serial.printf("Wrote %d bytes to LittleFS\n", bytes);
    f.close();
  }
}
