const int DIR_PIN = D5;
const int PUL_PIN = D6;
// const to toggle the direction, changing between HIGH and LOW changes the direction it spins
const int DIRECTION = HIGH;
// delay rate for how often the motor toggles PUL to move (affects speed)
const int DELAY_RATE_MS = 1;

// global variable to handle timing
int g_time_since_delay_ms = millis();
// state for the movement pin
int g_pul_pin_toggle = HIGH;
// state to determine if the motor is going or idle, the final program will need to toggle this on and off
bool g_going = true;

void setup() {
  // put your setup code here, to run once:
  pinMode(DIR_PIN, OUTPUT);
  pinMode(PUL_PIN, OUTPUT);
  // where the direction the motor spins is decided
  digitalWrite(DIR_PIN, DIRECTION);
  Serial.begin(115200);
  if(DELAY_RATE_MS > 3){
    // this is the only noticable error I can think of, hardware is likely going to be the cause of any issues with this and the program will not be able to detect those issues
    Serial.printf("Error: Motor moving too slow, choose a smaller delay rate \n");
    g_going = false;
  }
}

void loop() {
  // checks if the motor should be moving
  if(g_going){
    // checks whether a toggle should happen at a given time
    if(millis() - g_time_since_delay_ms >= DELAY_RATE_MS){
      // uses a state based toggle for moving the pin
      digitalWrite(PUL_PIN, g_pul_pin_toggle);
      g_pul_pin_toggle = !g_pul_pin_toggle;
      g_time_since_delay_ms = millis();
    }
  }
}

