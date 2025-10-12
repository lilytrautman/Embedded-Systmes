/**
 * This program will run an IR sensor connected to a servo which pans a room. Can be in either auto or manual mode
 * This module has four states: 
 * - Idle: servo is not moving and sensor is not getting readings, waiting for the start command.
 * - Calibrating: first state of auto mode, this will pan the room to calculate a threshold value for the scanning state to use. 
 * - Scanning: this will use the threshold from the calibrating state to pan the room to detect.
 * - Manual: this will have a manually set threshold and angle to point at to dectect. 
*/

#include <Adafruit_AMG88xx.h> //for IR sensor
#include <ESP32Servo.h> // for servo
#include <ArduinoJson.h> // for serial JSON communication 

enum State{
  CALIBRATING,
  IDLE,
  SCANNING,
  MANUAL
};

const int SERVO_PIN = D6;
const int BAUD_RATE = 115200;
const int ORIGIN_DEG = 0;
const int MAX_DEG = 180;
const String MODULE_NAME = "scanning_ir_camera";
const String UNIQUE_ID = MODULE_NAME + "1";
const String BRAIN_NAME = "*";

// max and min values 
const int CALIBRATION_TIME_MIN_MS = 1000;
const int CALIBRATION_TIME_MAX_MS = 100000;
const int SAMPLE_RATE_MIN_MS = 100; // fastest rate of IR camera sensing (10Hz)
const int SAMPLE_RATE_MAX_MS = 10000; 
const int MANUAL_ANGLE_MIN = 0;
const int MANUAL_ANGLE_MAX = 180;
const float MANUAL_THRESHOLD_MIN = 1.0;
const float MANUAL_THRESHOLD_MAX = 50.0;
const int FREQUENCY_RATE_MIN_MS = 500;
const int FREQUENCY_RATE_MAX_MS = 10000;
const float SMOOTHING_AMOUNT_MIN = 0.0;
const float SMOOTHING_AMOUNT_MAX = 10.0;

//configuration variables: (all set to default values)
int g_calibration_time_ms = 36000;
int g_scan_duration_ms = 18000;
int g_sample_rate_ms = SAMPLE_RATE_MIN_MS;
String g_mode = "auto";
int g_manual_angle = 90;
float g_manual_threshold = 25.0;
int g_frequency_rate_ms = 1000;
float g_smoothing_amount = 2.0;

//All other gloabl variables
float g_pixels[AMG88xx_PIXEL_ARRAY_SIZE];
int g_current_servo_angle = g_manual_angle;
unsigned long g_calibration_start_time_ms; 
Adafruit_AMG88xx g_ir_sensor; 
Servo g_myservo;
State g_state = IDLE; 

/**
 * helper method to move servo to and from servo_origin_deg to servo_max and back
 *
 * @param servo_origin_deg, servo_max
 * @return current degree of servo
 */
int move_servo_to_from(int servo_origin_deg, int servo_max);

/**
 * helper method to move servo to a single position at deg
 *
 * @param deg degree to move to 
 * @return current degree of servo
 */
int move_servo_to_deg(int deg);

/**
 * helper method to use get IR values and average value from the sensor. 
 * 
 * @return float which is the average of all of the IR sensor values
 */
float get_IR_values_avg();

/**
 * performs scanning state jobs.
 * 
 */
void scanning(unsigned long current_time_ms, float threshold);

/** 
 * performs calibrating state jobs.
 * 
 */
float calibrating(unsigned long current_time_ms);

/**
 * performs manual state jobs.
 * 
 */
void manual(unsigned long current_time_ms);

/**
 * Helper method to verify if deserialized json document has errors.
 * Checks for all required JSON document parts (mtype, to, from)
 * 
 * @param error that is returned from using deserializeJson().
 * @return if no errors have happened
 */
bool handle_errors(DeserializationError error, JsonDocument doc);

/**
 * will read serial input
 * 
 * @return void
 */
void read_serial_input();

/**
 * This will read the JSON by using various helper methods.
 * 
 * @return if properly read
 */
bool read_json(JsonDocument doc);

/**
 * will handle all JSON with mtype: "config"
 * 
 * @return bool if handled correctly
 */
bool input_config(JsonDocument doc);

/**
 * will handle all JSON with mtype: ".scan"
 * 
 * @return bool if handled correctly
 */
bool input_scan(JsonDocument doc);


/**
 * will handle all JSON with mtype: ".get"
 * 
 * @return bool if handled correctly
 */
bool input_get(JsonDocument doc);

/**
 * overloaded method to check for errors in a JSON message based on specific name and if it is within the range of allowed values
 * 
 * @param real_name the name that is specified in the documentation
 * @return bool if there is no issue with the name or value returns true, otherwise false
 */
bool check_error(String name, int value, int min, int max);
bool check_error(String name, float value, float min, float max);

/**
 * Method to send a JSON over serial.
 * 
 * Overloaded to support various types of values. 
 */
void send_json(String mtype, String name, String value);
void send_json(String mtype, String name, float g_pixels[]);
void send_json(String mtype, String name, float value);
void send_json(String mtype, String name, int value);

/**
 * Method to send a JSON debug over serial.
 *  
 */
void send_json_debug(String name, String value);

/**
 * Method to send a error over serial.
 *  
 */
void send_json_error(String name, String value);

void setup() {
  Serial.begin(BAUD_RATE);

  bool status = g_ir_sensor.begin();
  if (!status) {
    send_json_error("IR_camera_error", "There is an IR camera error. Cannot start.");
    while(1);
  }

  g_myservo.attach(SERVO_PIN);
  if(!g_myservo.attached()){
    send_json_error("Servo_error", "There is a servo error. Cannot start.");
    while(1);
  }

  delay(100); // let sensor boot up. This was in IR sensor's manufacturer's code so I implemented it here.

}

void loop() {
  static unsigned long s_last_sample_time_ms;
  static float s_threshold; // this will become the level at which some heat signature is detected if in auto
  
  unsigned long current_time_ms = millis();

  if(Serial.available() > 0){ 
   read_serial_input();
  }

  if(current_time_ms - s_last_sample_time_ms > g_sample_rate_ms){
    switch(g_state){
      case IDLE:
        break;
      case SCANNING:
        scanning(current_time_ms, s_threshold);
        break;
      case CALIBRATING:
        s_threshold = calibrating(current_time_ms);
        break;
      case MANUAL:
        manual(current_time_ms);
        break;
    }
    s_last_sample_time_ms = current_time_ms;
  }
}

float get_IR_values_avg(){
  g_ir_sensor.readPixels(g_pixels); // getting 8x8 IR values in 64 long float[]
  float average_read = 0.0;
  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    average_read += g_pixels[i];
  }
  return average_read / AMG88xx_PIXEL_ARRAY_SIZE;
}

int move_servo_to_from(int servo_origin_deg, int servo_max){
  static int s_servo_pos_deg = servo_origin_deg;
  static bool s_is_servo_increasing = false;
  move_servo_to_deg(s_servo_pos_deg);
  if (s_servo_pos_deg <= servo_origin_deg) { 
    s_is_servo_increasing = true;
  }
  if (s_is_servo_increasing) {
    s_servo_pos_deg++;
  }
  if (s_servo_pos_deg >= servo_max) { 
    s_is_servo_increasing = false;
  }
  if (!s_is_servo_increasing){ 
    s_servo_pos_deg--;
  }
  return s_servo_pos_deg;
}

int move_servo_to_deg(int deg){
  g_myservo.write(deg);
  return deg;
}

void scanning(unsigned long current_time_ms, float threshold){
  static unsigned long s_last_detection_time_ms;
  static float s_average_IR;
  g_current_servo_angle = move_servo_to_from(ORIGIN_DEG, MAX_DEG);
  s_average_IR = get_IR_values_avg();

  if (s_average_IR > threshold && (current_time_ms - s_last_detection_time_ms) > g_frequency_rate_ms ){ //above threshold and long enough since last message
    send_json("motion_detected","motion_detected", g_current_servo_angle); // chose to get the angle this way instead of having a global servo array variable, this might cause ~1 degree of error due to servo inaccuracies
    s_last_detection_time_ms = current_time_ms;
  }
}

float calibrating(unsigned long current_time_ms){
  static float s_average_IR;
  static float s_threshold = 0.0;
  g_current_servo_angle = move_servo_to_from(ORIGIN_DEG, MAX_DEG);
  s_average_IR = get_IR_values_avg();
  s_threshold += s_average_IR;

  if(current_time_ms > g_calibration_start_time_ms + g_calibration_time_ms){
    g_state = SCANNING;
    s_threshold = (s_threshold / (g_calibration_time_ms/g_sample_rate_ms)) + g_smoothing_amount; // getting average reading for time in calibration state
    send_json_debug("finished_calibrating", "Calibrated threshold is: " + String(s_threshold));
    
  }
  return s_threshold;
}

void manual(unsigned long current_time_ms){
  static unsigned long s_last_detection_time_ms;
  static float s_average_IR;
  g_current_servo_angle = move_servo_to_deg(g_manual_angle);
  s_average_IR = get_IR_values_avg();
  if (s_average_IR > g_manual_threshold && (current_time_ms - s_last_detection_time_ms) > g_frequency_rate_ms ){ //above threshold and long enough since last message
    send_json("motion_detected","motion_detected", g_current_servo_angle); // chose to get the angle this way instead of having a global servo array variable, this might cause ~1 degree of error due to servo inaccuracies
    s_last_detection_time_ms = current_time_ms;
  }
}

void read_serial_input(){
  static String s_json_string = "";
  char serial_read_char = Serial.read();
  if(serial_read_char == '\n' || serial_read_char == '\r'){ // if it is an line ending, you can deserialize
    if(!s_json_string.isEmpty()){ // catch for new line and carriage return so does not deserialize emptiness
      JsonDocument doc; 
      DeserializationError error = deserializeJson(doc, s_json_string);
      if(handle_errors(error, doc)){
        if(read_json(doc)){
          send_json_debug("successful_command", ("mtype: '" + String(doc["mtype"]) + "' name: '" + String(doc["name"]) + "' value: '" + String(doc["value"]) + "' ran successfully." ));
        }
      }
      s_json_string = ""; // resetting String after it has been used
    }
  } else {
    s_json_string.concat(serial_read_char); //adding character to String if it is not line ending
  }
}

bool read_json(JsonDocument doc) {
  String mtype = doc["mtype"];
  if(mtype.compareTo("config") == 0){ //send JSON to correct place depending on mtype.
    return input_config(doc);
  } else if(mtype.compareTo((MODULE_NAME +".scan")) == 0){
    return input_scan(doc);
  } else if(mtype.compareTo((MODULE_NAME +".get"))==0){
    return input_get(doc);
  }
  send_json_error("JSON_error", "Unsupported 'mtype' value: " + mtype);
  return false;
}

bool handle_errors(DeserializationError error, JsonDocument doc){
  if (error) {
    String temp_s = "Deserialization failed: ";
    send_json_error("JSON_error", temp_s + String(error.c_str()));
    return false;
  }

  if (!doc.containsKey("mtype")) {
    send_json_error("JSON_error", "JSON does not contain 'mtype' key.");
    return false;
  }

  if (!doc.containsKey("name")) {
    send_json_error("JSON_error", "JSON does not contain 'name' key.");
    return false;
  }

  if (!doc.containsKey("from")) {
    send_json_error("JSON_error", "JSON does not contain 'from' key");
    return false;
  }

  String from = doc["from"];
  if (from.compareTo(BRAIN_NAME) != 0) {
    send_json_debug("JSON_debug", "Unsupported 'from' value: " + from);
    return false;
  }

  if (!doc.containsKey("to")) {
    send_json_error("JSON_error", "JSON does not contain 'to' key.");
    return false;
  }
  
  String to = doc["to"];
  if (to.compareTo(UNIQUE_ID) != 0) {
    send_json_debug("JSON_debug", "Unsupported 'to' value: " + to);
    return false;
  }

  return true;
}

bool input_config(JsonDocument doc){
  if (!doc.containsKey("value")) {
    send_json_error("JSON_error", "JSON does not contain 'value' key.");
    return false;
  }

  String name = doc["name"];
  String value = doc["value"];

  int temp_i = doc["value"].as<int>();
  if(name.compareTo("calibration_time_ms")==0){
    if(check_error("calibration_time_ms", temp_i, CALIBRATION_TIME_MIN_MS, CALIBRATION_TIME_MAX_MS)){
      g_calibration_time_ms = temp_i;
      return true;
    }
    return false;
  }else if (name.compareTo("sample_rate_ms") == 0){
    if(check_error("sample_rate_ms", temp_i, SAMPLE_RATE_MIN_MS, SAMPLE_RATE_MAX_MS)){
      g_sample_rate_ms = temp_i;
      return true;
    }
    return false;
  }else if (name.compareTo("manual_angle") == 0){
    if(check_error("manual_angle", temp_i, MANUAL_ANGLE_MIN, MANUAL_ANGLE_MAX)){
      g_manual_angle = temp_i;
      return true;
    }
    return false;
  }else if (name.compareTo("frequency_rate_ms") == 0){
    if(check_error("frequency_rate_ms", temp_i, FREQUENCY_RATE_MIN_MS, FREQUENCY_RATE_MAX_MS)){
      g_frequency_rate_ms = temp_i;
      return true;
    }
    return false;
  }

  float temp_f = doc["value"].as<float>();
  if (name.compareTo("manual_threshold") == 0){
    if(check_error("manual_threshold", temp_f, MANUAL_THRESHOLD_MIN, MANUAL_THRESHOLD_MAX)){
      g_manual_threshold = temp_f;
      return true;
    }
    return false;
  } else if (name.compareTo("smoothing_amount") == 0){
    if(check_error("smoothing_amount", temp_f, SMOOTHING_AMOUNT_MIN, SMOOTHING_AMOUNT_MAX)){
      g_smoothing_amount = temp_f;
      return true;
    }
    return false;
  }

  if(name.compareTo("mode") == 0){
    if(value.compareTo("manual")==0){
      g_mode = "manual";
      if(g_state != IDLE){
        g_state = MANUAL;
      }
      return true;
    }else if(value.compareTo("auto")==0){
      g_mode = "auto";
      if(g_state == MANUAL){
        g_calibration_start_time_ms = millis();
        g_state = CALIBRATING;
      }
      return true;
    }else{
      send_json_error("JSON_error", "Unsupported 'mode' value: " + value);
      return false;
    }
  }
  send_json_error("JSON_error", "Unsupported 'name' value: " + name);
  return false;
}

bool input_scan(JsonDocument doc){
  String name = doc["name"];
  if(name.compareTo("start") == 0){
    if(g_mode == "manual"){
      g_state = MANUAL;
    }else{
      g_state = CALIBRATING;
      g_calibration_start_time_ms = millis();
    }
    return true;
  }else if(name.compareTo("stop")== 0){
    g_state = IDLE;
    return true;
  }
  send_json_error("JSON_error", "Unsupported 'name' value: " + name);
  return false;
}


bool input_get(JsonDocument doc){
  String name = doc["name"];
  float avg_IR = get_IR_values_avg();
  if(name.compareTo("heatmap") == 0){
    send_json("out", "heatmap", g_pixels);
    return true;
  }else if(name.compareTo("angle")== 0){
    send_json("out", "angle", g_current_servo_angle);
    return true;
  }else if(name.compareTo("average_ir") == 0){
    send_json("out", "average_ir", avg_IR);
    return true;
  }
  send_json_error("JSON_error", "Unsupported 'name' value: " + name);
  return false;
}

bool check_error(String name, int value, int min, int max){
  if(value < min || value > max){
    send_json_error("JSON_error", "Unsupported '" + name + "' value: " + String(value));
    return false;
  }
  return true;
}

bool check_error(String name, float value, float min, float max){
  if(value < min || value > max){
    send_json_error("JSON_error", "Unsupported '" + name + "' value: " + String(value));
    return false;
  }
  return true;
}

void send_json(String mtype, String name, String value){
  JsonDocument doc;
  doc["mtype"] = MODULE_NAME + "." + mtype;
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  doc["value"] = value;
  serializeJson(doc, Serial);
  Serial.println();
}

void send_json(String mtype, String name, int value){
  JsonDocument doc;
  doc["mtype"] = MODULE_NAME + "." + mtype;
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  doc["value"] = value;
  serializeJson(doc, Serial); 
  Serial.println();
}

void send_json(String mtype, String name, float g_pixels[]){
  JsonDocument doc;
  doc["mtype"] = MODULE_NAME + "." + mtype;
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  JsonArray values = doc["value"].to<JsonArray>();
  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    values.add(g_pixels[i]);
  }
  serializeJson(doc, Serial); 
  Serial.println();
}

void send_json(String mtype, String name, float value){
  JsonDocument doc;
  doc["mtype"] = MODULE_NAME + "." + mtype;
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  doc["value"] = value;
  serializeJson(doc, Serial); 
  Serial.println();
}

void send_json_debug(String name, String value){
  JsonDocument doc;
  doc["mtype"] = "debug";
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  doc["message"] = value;
  serializeJson(doc, Serial);  
  Serial.println();
}

void send_json_debug(String name, float value){
  JsonDocument doc;
  doc["mtype"] = "debug";
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  doc["message"] = value;
  serializeJson(doc, Serial);
  Serial.println();
}

void send_json_error(String name, String value){
  JsonDocument doc;
  doc["mtype"] = "error";
  doc["from"] = UNIQUE_ID;
  doc["to"] = BRAIN_NAME;
  doc["name"] = name;
  doc["message"] = value;
  serializeJson(doc, Serial);  
  Serial.println();
}