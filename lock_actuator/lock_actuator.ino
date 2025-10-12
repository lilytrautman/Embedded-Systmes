/*
* lock_actuator.ino
*
* Reads JSON messages to control a lock actuator
*
* Author: Sage Labesky
* Created: 5/20/2025
* Modified: 5/26/2025
*/
#include <ArduinoJson.h>

const int DIR_PIN = D4;
const int PUL_PIN = D5;
const float MICROSTEPS_PER_ROTATION = 3200;
const float DEGREE_FOR_CALC = 360;

int g_direction = HIGH; // global direction to keep track of the direction the motor is moving
int g_num_microsteps = 1600; // keeps tack of the number of microsteps, defaults to 1600 which is 180 degrees
StaticJsonDocument<200> g_command; // global variable storing the command input
StaticJsonDocument<200> g_output; // global variable storing a json output
String g_json = String(); // global variable used to store the json information
bool g_locked = false; // state variable that keeps track of whether the lock is locked
bool g_lock_changing = false; // state variable that keeps track of whether the state is changing


void setup() {
  pinMode(DIR_PIN, OUTPUT);
  pinMode(PUL_PIN, OUTPUT);
  // where the direction the motor spins is decided
  digitalWrite(DIR_PIN, g_direction);
  Serial.begin(115200);
}

void loop() {
  // counter variable used in moving the lock
  static int count = 0;
  bool command_ready = readJson(); // when a command is ready loop will do the command
  if (command_ready){
    do_command();
    command_ready = false;
  }
  if (g_lock_changing){ // if the lock is changing it will move the lock
    count = lock_change(count);
  }
}

bool readJson() { // function that reads input from serial to build a json command
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      // attempts to parse the JSON
      DeserializationError error = deserializeJson(g_command, g_json);
      g_json = "";  // always clears string after trying to parse
      if (error) { // either errors out or if the command is actually meant for the module it is executed
        send_output("error", error.c_str());
        return false;
      } else if(g_command["to"] == "lock_actuator1"){
        return true;
      }
    } else {
      g_json += c;
    }
  }
  return false; // returns false by default if a command is not ready
}

/*
do_command function executes a command. It does this by checking the mtype and then calling the specific helper method tied to that command
parameters: none
Output: none
*/
void do_command(){
  String mtype = g_command["mtype"] | "error"; // mtype determines command
  // conditional to check every valid command and error if the command is not valid
  if(mtype == "error"){
    send_output("error","Error, mtype is missing in command");
  } else if(mtype == "config"){
    apply_config_values();
  } else if(mtype == "lock_actuator1.lock"){
    if(g_locked == false){ // lock should only lock if not already locked
      g_lock_changing = true;
      digitalWrite(DIR_PIN, HIGH);
    } else {
      send_output("error", "lock_actuator1 is already locked");
    }
  } else if(mtype == "lock_actuator1.unlock"){
    if(g_locked == true){ // lock should only unlock if not already unlocked
      g_lock_changing = true;
      digitalWrite(DIR_PIN, LOW);
    } else {
      send_output("error", "lock_actuator1 is already unlocked");
    }
  } else if(mtype == "lock_actuator1.request_state"){
    String message = "State of lock, locked = " + String(g_locked);
    send_output("lock_actuator1.state",message);
  } else {
    send_output("error", "error, no mtype command matches given mtype");
  }
  g_command.clear(); // clears the command after choosing one to execute
}

/*
apply_config_values function is called when mtype is config. It applies the config value.
There is a major deviation from the readme here. Instead of having locked_position and unlocked_position, there is only degree_to_lock and it represents
how many steps the device should move while locking (and unlocking)
parameters: none
Output: none
*/
void apply_config_values(){
  String name = g_command["name"] | "error";
  String value = g_command["value"] | "error";
  if(name == "error" || value == "error"){
    send_output("error","error, name or value of config parameter was not provided");
  } else if(name == "degree_to_lock" && g_locked == false && g_lock_changing == false){ // only changes the locked position if not locked or moving
    int new_degree = int(value.toFloat()*(MICROSTEPS_PER_ROTATION/DEGREE_FOR_CALC));  // math to convert the degree value of the rotation into microsteps
    if (new_degree <= 0){ // handles both negative and non int/float inputs
      send_output("error", "Invalid Degree entered");
    } else{
          g_num_microsteps = new_degree;
    }
  } else {
    send_output("error","Error, lock_actuator1 config name does not match accepted name or device is locked");
  }
}

/*
lock_change function handles the movement of the lock. it completes one step of the motor using delayMicroseconds
parameters: one int: count
Output: one int: count
*/
int lock_change(int count){
  count+=1; // count is used here to keep track of how many microsteps the motor has completed
  if (count <= g_num_microsteps && count >= 0){ // it completes the specified number of microsteps from the degree_to_lock config
    // stepper motor works by pulsing between low and high
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(50);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(50);
  } else if (count >= g_num_microsteps) {
    // code to handle completion of movement
    count = 0;
    g_lock_changing = false;
    g_locked = !g_locked;
    // outputs state change
    String message = "State of lock changed, locked = " + String(g_locked);
    send_output("lock_actuator1.state_change", message);
  }
  return count;
}

/*
send_output command handles all outputs from this device in json format
parameters: none
Output: none
*/
void send_output(String mtype, String message){
  // mtype and the message are the only things that change depending on the message
  g_output["mtype"] = mtype;
  g_output["from"] = "lock_actuator1";
  g_output["to"] = "*";
  g_output["message"] = message;
  serializeJson(g_output, Serial);
  
  Serial.printf("\n"); // this is specifically for human reading. Without it the json output messages all print on the same line
}