#include <ArduinoJson.h>


enum State{
  STOP,
  RUNNING,
  ALERT
};

// Constants
const int SENSOR_PIN = D7;
const String UID = "vibration_sensor1";
const String BRAIN_UID = "*";
const int MAX_SMOOTHING = 20; // hard limit for size of the buffer containing vibration history


// Global defaults & config parameters
State g_state;                      // holds the behavioral state of the sensor
int g_sample_rate_hz = 1;           // rate at which digitalRead() is called and output is broadcasted
int g_threshold;                    // digitalRead() is called 100 times; this var dictates what % of those calls need to return true to count as a positive reading
int g_baseline_vibration;           // number of times out of 100 that digitalread returns low
unsigned long g_last_sample_ms;
bool g_vib_history[MAX_SMOOTHING]; 
int g_history_index = 0;            // keeps track of what index in the array containing vibration history
int g_smoothing_window = 5;         // config variable: size of vibration history buffer
int g_broadcast_rate_hz = 1;        // config variable that determines the broadcast rate of sensor state when in RUNNING state


// Function declarations
void read_sensor();
void check_for_json();
void process_json(String mtype, String param_name, int value);
void send_debug_msg(String debug_msg);
void send_error_msg(String to_id, String error_msg);
void send_status_msg(String value);
bool is_valid_json(const JsonDocument& doc, String& error_message);
int get_vibration();


void setup() {
  pinMode(SENSOR_PIN, INPUT); // set up sensor pin for digital input
  Serial.begin(115200);
  delay(50);
  g_baseline_vibration = get_vibration(); // establish a baseline vibration 
  g_threshold = g_baseline_vibration + 5; 
  g_state = STOP; // start sensor in running state (this will change to start at stopped state and change to running or alert when a JSON message is received)
}

// First, it will check if there is a JSON message available, then perform functions based on g_state
void loop() {
  check_for_json();
  read_sensor();
}


// Do the main function of the sensor
// Read from the sensor then broadcast the result based on the current state
// This also includes smoothing
void read_sensor(){
  static unsigned long last_sample_ms = 0; // holds last time at which it sampled last
  static unsigned long last_broadcast_ms = 0;
  unsigned long currentTime = millis(); // capture current time
  unsigned long sample_interval_ms = 1000 / g_sample_rate_hz;
  unsigned long broadcast_interval_ms = 1000 / g_broadcast_rate_hz;

  // Check if it's time to sample
  if(currentTime - last_sample_ms < sample_interval_ms) return;
  last_sample_ms = currentTime;

  // Get vibration data
  int count = get_vibration();
  int delta = count - g_baseline_vibration;
  bool is_vibrating = (delta > g_threshold); // only counts as vibrating if its above a threshold

  // Add to history buffer & increment
  g_vib_history[g_history_index] = is_vibrating;
  g_history_index = (g_history_index + 1) % g_smoothing_window; 

  // count the last x readings, 
  int num_positives = 0;
  for(int i = 0; i < g_smoothing_window; i++){
    if(g_vib_history[i]) num_positives++;
  }

  // If the more than half of the past x samples are valid vibrations, smooth_vib will be true
  bool smooth_vib = (num_positives > g_smoothing_window / 2);

  //Send status message, do nothing if in "stop" state
  if(g_state == RUNNING){
    if(!(currentTime - last_broadcast_ms < broadcast_interval_ms)){
        send_status_msg(smooth_vib);
        last_broadcast_ms = currentTime;
    }
  }
  else if (g_state == ALERT && smooth_vib){
    send_status_msg(true);
  }
}

// Check for JSON message that are addressed to "door_sensor_unique_id"
// These JSON messages will include configuration parameters or a command to switch the state of the sensor
void check_for_json(){
  static String s_json_msg = ""; // json string buffer
  static char s_last_char = 0; // last char to help avoid processing double jsons with NL and CR
  StaticJsonDocument<256> doc; 
  if(!Serial.available()) return;

  char c = Serial.read();

  // Skip second char of CRLF or LFCR
  if ((c == '\n' && s_last_char == '\r') || (c == '\r' && s_last_char == '\n')) {
    s_last_char = c;
    return;
  }

  // continue building buffer
  s_json_msg.concat(c);
  if(c != '\n' && c != '\r'){
    s_last_char = c;
    return;
  }

  // Check for deserialize error, send error msg
  DeserializationError error = deserializeJson(doc, s_json_msg);
  if(error){
    send_error_msg("*", "Deserialization failed" + String(error.c_str()));
    s_json_msg = "";
    return;
  }

  // Ensure doc is a JsonObject - when the json is deserialized, it is a JsonVariant in the form of a <JsonObject>, <JsonArray>, <const char*>, <bool>, <long>, etc.???????
  JsonVariant root = doc.as<JsonVariant>();
  if(!root.is<JsonObject>()){
    send_error_msg("*", "Error: Received JsonVariant that isn't a JsonObject");
    s_json_msg = "";
    return;
  }

  // Check "to" value
  if(!doc["to"].is<const char*>()) return; // Make sure we can cast first
  String to = doc["to"].as<String>();

  // Check that "to" = UID or "*" (we will process either)
  if(to != UID && to != "*"){
    s_json_msg = "";
    return;
  }

  // Handle config msg errors
  String error_msg;
  if(!is_valid_json(doc, error_msg)){
    send_error_msg(BRAIN_UID, error_msg);
    s_json_msg = "";
    return;
  }

  // Process config or input msg
  process_json(doc["mtype"], doc["name"], doc["value"]);
  s_json_msg = ""; // Clear buffer
}

// Process JSON mtype, name, and value and send JSON debug response
void process_json(String mtype, String param_name, int value){
  if (mtype == "config"){ // config message
    if(param_name == "sample_rate"){
      g_sample_rate_hz = value;
    }
    else if (param_name == "threshold"){
      g_threshold = value;
    }
    else if (param_name == "baseline_vibration"){
      if(g_threshold <= g_baseline_vibration){
        g_threshold = value + 5;
      }
      g_baseline_vibration = value;
    }
    else if (param_name == "smoothing"){
      g_smoothing_window = value;
    }
    else if (param_name == "broadcast_rate"){
      g_broadcast_rate_hz = value;
    }
  }

  // Prepare and send debug message
  if(mtype == "config"){
    String debug_msg = ("Successfully changed " + param_name + " to " + value);
    send_debug_msg(debug_msg);
  }
}

// Send debug msg with debug message
void send_debug_msg(String debug_msg){
  String output;
  StaticJsonDocument<256> debug_doc;
  debug_doc["mtype"] = "debug";
  debug_doc["to"] = BRAIN_UID;
  debug_doc["from"] = UID;
  debug_doc["message"] = String(debug_msg);
  serializeJsonPretty(debug_doc, Serial);
}

// Send error message with error message
void send_error_msg(String to_id, String error_msg){
  String output;
  StaticJsonDocument<256> error_doc;
  error_doc["mtype"] = "error";
  error_doc["to"] = to_id;
  error_doc["from"] = UID;
  error_doc["message"] = error_msg;
  serializeJsonPretty(error_doc, Serial);
}

// Broadcast output message with is_vibrating value
void send_status_msg(bool value){
  String output;
  StaticJsonDocument<256> status_doc;
  if(g_state == ALERT){
    status_doc["mtype"] = "vibration_sensor.alert_output";
  }else{
    status_doc["mtype"] = "vibration_sensor.output";
  }
  status_doc["to"] = BRAIN_UID;
  status_doc["from"] = UID;
  status_doc["name"] = "is_vibrating";
  status_doc["value"] = value;
  serializeJsonPretty(status_doc, Serial);
}

bool is_valid_json(const JsonDocument& doc, String& error_msg) {
  error_msg = "Error: ";
  bool valid = true;

    // ------- Required keys ---------
  if (!doc.containsKey("mtype")) {
    error_msg += "Missing 'mtype' key. ";
    valid = false;
  }
  if (!doc.containsKey("to")) {
    error_msg += "Missing 'to' key. ";
    valid = false;
  }
  if (!doc.containsKey("from")) {
    error_msg += "Missing 'from' key. ";
    valid = false;
  }
  if (!valid) return false; // all messages must have these keys

  // --- Type checks for required keys ---
  if (!doc["mtype"].is<const char*>()) {
    error_msg += "'mtype' must be a string. ";
    valid = false;
  }
  if (!doc["to"].is<const char*>()) {
    error_msg += "'to' must be a string. ";
    valid = false;
  }
  if (!doc["from"].is<const char*>()) {
    error_msg += "'from' must be a string. ";
    valid = false;
  }
  if (!valid) return false;

  // ----- Make sure mtypes are correct ------
  String mtype = doc["mtype"].as<String>();
  if (mtype != "config" &&
    mtype != "vibration_sensor.start" &&
    mtype != "vibration_sensor.stop" &&
    mtype != "vibration_sensor.alert") {
    error_msg += "Invalid mtype value. ";
    return false;
  }

  // --- Handle state change commands ---
  if (mtype == "vibration_sensor.start") {
    g_state = RUNNING;
    send_debug_msg("vibration_sensor changed to RUNNING state");
    return true;
  }
  if (mtype == "vibration_sensor.stop") {
    g_state = STOP;
    send_debug_msg("vibration_sensor changed to STOP state");
    return true;
  }
  if (mtype == "vibration_sensor.alert") {
    g_state = ALERT;
    send_debug_msg("vibration_sensor changed to ALERT state");
    return true;
  }

  // --- Config messages must have name and value ---
  if (!doc.containsKey("name")) {
    error_msg += "Missing 'name' key. ";
    valid = false;
  }
  if (!doc.containsKey("value")) {
    error_msg += "Missing 'value' key. ";
    valid = false;
  }
  if (!valid) return false;

  // --- Type checks for config fields ---
  if (!doc["name"].is<const char*>()) {
    error_msg += "'name' must be a string. ";
    valid = false;
  }
  if (!doc["value"].is<int>()) {
    error_msg += "'value' must be an integer. ";
    valid = false;
  }
  if (!valid) return false;

  // --- Specific value checks ---
  String name = doc["name"].as<String>();
  int value = doc["value"].as<int>();

  if (name != "sample_rate" && name != "baseline_vibration" &&
      name != "threshold" && name != "smoothing" && name != "broadcast_rate") {
    error_msg += "Invalid 'name' value. ";
    valid = false;
  }

  if ((name == "threshold" || name == "baseline_vibration") && (value < 0 || value > 100)) {
    error_msg += "'" + name + "' must be between 0 and 100. ";
    valid = false;
  }

  if (name == "smoothing" && (value < 1 || value > 20)) {
    error_msg += "'smoothing' must be between 1 and 20. ";
    valid = false;
  }
  if(name == "sample_rate" && (value < 1 || value > 50)){
    error_msg += "'sample_rate' must be between 1 and 50. ";
    valid = false;
  }
  if(name == "broadcast_rate" && (value < 1 || value > 10)){
    error_msg += "'broadcast_rate' must be between 1 and 10. ";
    valid = false;
  }

  return valid;
}

// Returns the number of vibrations captured in 100 digitalRead calls
int get_vibration(){
  int num_positives = 0;
  for(int i = 0; i < 100; i++){
    if(digitalRead(SENSOR_PIN) == LOW){
      num_positives++;
    }
    delayMicroseconds(100);
  }
  return num_positives;
}





