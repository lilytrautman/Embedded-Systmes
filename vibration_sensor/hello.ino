#include <ArduinoJson.h>

enum State{
  STOP,
  RUNNING,
  ALERT
};
const int SENSOR_PIN = D7;
State g_state; // holds the behavioral state of the sensor
boolean g_sig_state; // holds the phsyical state of the sensor
int g_broadcast_rate_hz; // determines how often to broadcast the sensor state if in RUNNING mode
unsigned long g_last_broadcast_ms; // captures the time when it broadcasted last
  

void setup() {
  pinMode(SENSOR_PIN, INPUT); // set up sensor pin for digital input
  Serial.begin(115200);
  g_state = RUNNING; // start sensor in running state (this will change to start at stopped state and change to running when a JSON message is received)
  g_sig_state = 0;
  g_broadcast_rate_hz = 1; // changing this variable will change broadcast rate of sensor
  g_last_broadcast_ms = 0; 

}

// First, it will check if there is a JSON message available, then perform functions based on g_state
void loop() {
  // Check for JSON message that are addressed to "door_sensor_unique_id"
  // These JSON messages will include configuration parameters or a command to switch the state of the sensor

  // After getting a JSON message, we will handle errors, which include invalid JSON keys, types, values, etc of input parameters
  if (g_broadcast_rate_hz <= 0) {
    Serial.println("ERROR: Invalid broadcast rate: Must be > 0.");
    g_broadcast_rate_hz = 1; // set to default
  }

  g_sig_state = digitalRead(SENSOR_PIN); // get state of sensor
  if(g_sig_state != HIGH && g_sig_state != LOW){ // invalid reading from the sensor
    Serial.println("ERROR: Sensor returned an invalid state");
  }

  unsigned long currentTime = millis(); // capture current time
  if (g_state == RUNNING){ // in the running state, we will continuously broadcast the state of the vibration sensor
    unsigned long latency = (1000 / g_broadcast_rate_hz);
    if (currentTime - g_last_broadcast_ms >= latency){ // check if it's time to send a message
      if(g_sig_state == LOW){
        Serial.printf("vibration detected \n"); // these printf's will eventually be JSON messages
      }else{
        Serial.printf("vibration not detected \n");
      }
      g_last_broadcast_ms = currentTime; // update broadcast time
    }
  }
  else if (g_state == ALERT){ // in the alarm state, we will broadcast an alert only if the sensor detects vibration
    if (g_sig_state == LOW){
      Serial.printf("Vibration detected!!\n");
    }
  }
  else{  // in the stopped state, the sensor will do nothing
  }
}
