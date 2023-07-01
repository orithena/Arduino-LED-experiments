

void slider_callback(Control* sender, int type) {
  int value = atoi(sender->value.c_str());
  Serial.printf("id: %d\tval: %d\tsender.type: %d\ttype: %d\n", sender->id, value, sender->type, type);
  state.light = ui_to_val(value);
  ESPUI.updateSlider(0, val_to_ui(state.light), -1);
}

void save_callback(Control* sender, int type) {
  if( type > 0 ) {
    save_state();
    print_state();
  }
}

void pattern_callback(Control* sender, int type) {
  if( type > 0 ) {
    nextPattern();
    print_state();
  }
}

void static_callback(Control* sender, int type) {
  state.display_static = type > 0;
  print_state();
}

void setup_ui() {
  ESPUI.slider("Helligkeit", &slider_callback, COLOR_EMERALD, _max(0, _min(100, val_to_ui(state.light)) ));
  ESPUI.button("Effekte", &pattern_callback, COLOR_SUNFLOWER, "Nächster");
  ESPUI.switcher("Nächster Effekt alle 6 Minuten", &static_callback, COLOR_SUNFLOWER, !(state.display_static));
  ESPUI.button("Diese Einstellungen zum Standard machen", &save_callback, COLOR_ALIZARIN, "Speichern");
  ESPUI.begin("Emilys Unendliche Matrix");
}


void setup_wifi() {
  WiFiManager wm;
  wm.setHostname("emily");
  if( wm.autoConnect("Emilys Unendliche Matrix") ) {
    Serial.print("WiFi set up with IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error connecting to WiFi");
  }
}
