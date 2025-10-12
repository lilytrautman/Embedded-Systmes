/**
 * In this program, the servo sweeps from `s_origin_deg` and `s_max_deg` using `move_servo_to_from(int servo_origin_deg, int servo_max)` 
 * and the ability to move to a specific angle with `move_servo_to_deg(int deg)`. 
 * The IR sensor (AMG8833) is implemented so that it can successfully calibrate to room temperature, 
 * get the 64 element array (`float*`) representing the 8x8 thermal image, and decide if something is detected to prove functionality of the sensor. 
 * The whole system moves at `10Hz` because that is the max speed of the IR sensor. 
*/

#include <Adafruit_AMG88xx.h> //for IR sensor
#include <ESP32Servo.h> // for servo

Adafruit_AMG88xx g_ir_sensor;
Servo g_myservo;

const int SAMPLE_RATE_MS = 100; // max rate of IR camera sensing (10Hz)
const int SERVO_PIN = D6;

// States have been done in order to test if dectection of a human is possible and to further understand the hardware
enum State{
  CALIBRATING, //this will wait a certain amount of time to gather data and make a decision of what the threshold for detection is
  RUNNING // this will run the servo and IR camera, printing if something breaks the threshold
};

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
 * @param pixel_str so that we can send back the 8x8 grid of values from IR sensor in string format. "returns" this string to be printed if needed.
 * @return float which is the average of all of the IR sensor values
 */
float get_IR_values_avg(String &pixel_str);

/**
 * prints info from IR sensor and servo
 *
 * @param pixel_str, average_IR, servo_deg all values that need to be printed
 */
void print_info(String pixel_str, float average_IR, int servo_deg);

void setup() {
  Serial.begin(115200);
  bool status = g_ir_sensor.begin();
  if (!status) {
    Serial.printf("IR sensor error!\n");
    while (1);
  }
  g_myservo.attach(SERVO_PIN);
  if(!g_myservo.attached()){
    Serial.printf("Servo error!\n");
  }
  delay(100); // let sensor boot up. This was in IR sensor's manufacturer's code so I implemented it here.
}

void loop() {
  static State s_state = CALIBRATING;
  static unsigned long s_current_time_ms;
  static unsigned long s_last_sample_time_ms;
  static int s_origin_deg = 0;
  static int s_max_deg = 180;
  static int num_readings = 0;
  static float s_threshold; //this will become the level at which some heat signature is detected
  static float s_amount_of_noise = 2.0; // this is an extra level to smooth detector reading, 2.0 seems to be working quite well 
  static unsigned long s_calibration_time_ms = 10000; // time to calibrate for

  String pixel_str;
  float average_IR;
  int servo_deg;

  s_current_time_ms = millis();

  if(s_current_time_ms - s_last_sample_time_ms > SAMPLE_RATE_MS){
    servo_deg = move_servo_from_to(s_origin_deg, s_max_deg);
    average_IR = get_IR_values_avg(pixel_str);
    num_readings ++;

    switch(s_state){
      case RUNNING:
        if (average_IR > s_threshold){
          Serial.printf("DECTECTED SOMETHING!\n");
        }
        break;
      case CALIBRATING:
        s_threshold += average_IR;
        if(s_current_time_ms > s_calibration_time_ms){
          s_state = RUNNING;
          s_threshold = (s_threshold / num_readings) + s_amount_of_noise; // getting average reading for time in calibration state
          Serial.printf("Done Calibrating, threshold is: %f\n", s_threshold);
        }
        break;
    }
    print_info(pixel_str, average_IR, servo_deg);
    s_last_sample_time_ms = s_current_time_ms;
  }
  
}

int move_servo_from_to(int servo_origin_deg, int servo_max){
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

float get_IR_values_avg(String &pixel_str){
  float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

  g_ir_sensor.readPixels(pixels); // getting 8x8 IR values in 64 long float*
  pixel_str = "";
  float average_read = 0.0;

  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    pixel_str += String(pixels[i]) + " ";
    average_read += pixels[i];
    if((i+1)%8==0){
      pixel_str += "\n"; // to make string 8x8 when printed
    }
  }
  return average_read / AMG88xx_PIXEL_ARRAY_SIZE;
}

int move_servo_to_deg(int deg){
  g_myservo.write(deg);
  return deg;
}

void print_info(String pixel_str, float average_IR, int servo_deg){
  Serial.printf("average IR sensor reading: %f \n", average_IR);
  Serial.printf("current servo degree: %d\n", servo_deg);
  Serial.print("pixels:\n");
  Serial.print(pixel_str);
  Serial.println();
}
