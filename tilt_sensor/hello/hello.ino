//*****************************************************************************
// * File: hello.ino
// * Author: Lily Trautman
// * Created: May 7 2025
// * Modified: May 7 2025
// *
// * Brief: Demonstrates reading a SCA121T-D07 tilt sensor,
// *        applying a smoothing filter, converting to degrees,
// *        and issuing alerts based on tilt thresholds.
// *        Includes various test scenarios.
// *
// * Hardware: SCA121T-D07 Tilt Sensor
// *   - Red:    +5V
// *   - Blue:   GND
// *   - Yellow: X-axis output -> Arduino A0
// *   - Green:  Y-axis output -> Arduino A1
// *****************************************************************************/

// --- Constant Definitions ---
// Pin Assignments
const int X_AXIS_PIN = A0; //Yellow
const int Y_AXIS_PIN = A1; //Green

// Test Scenarios - SET ONE to choose a test configuration
enum TestScenario {
  TS_NORMAL_OP,
  TS_CONFIG_SMOOTHING,
  TS_CONFIG_FREQUENCY,
  TS_CONFIG_THRESHOLD,
  TS_RECALIBRATE_CMD,
  TS_STOP_CMD,
  TS_FORCE_CALIB_FAIL,
  TS_FORCE_TILT_ALERT
};
const TestScenario G_CURRENT_TEST_SCENARIO = TS_NORMAL_OP; // ****** CHOOSE TEST SCENARIO HERE ******

// Sensor & System Configuration
const int MAX_SMOOTHING_SAMPLES = 10;   // Max buffer size for smoothing filter
const float ADC_MAX_VALUE = 1023.0;     // Max value for a 10-bit ADC
const float VCC_VOLTAGE_V = 5.0;        // Arduino VCC voltage
const float DEGREES_PER_VOLT = 45.0;    //test value, should give range from -90 deg to 38.3 deg

// Sensor Electrical & Angular Characteristics for SCA121T-D03
// IMPORTANT: SCA121T-D03 is specified for 7-35V supply.
// Running at 5V is out of spec. Performance may vary from datasheet.
// Calibration is CRITICAL.
const bool SENSOR_MODEL_IS_D03 = true; // For clarity in code if needed later

const float SENSOR_MEASURING_RANGE_DEG = 90.0;    // Per datasheet for D03: +/-90 degrees
const float SENSOR_MIN_ANGLE_DEG = -SENSOR_MEASURING_RANGE_DEG;
const float SENSOR_MAX_ANGLE_DEG = SENSOR_MEASURING_RANGE_DEG;

// Nominal values from datasheet (specified for 7-35V supply for D03)
const float SENSOR_NOMINAL_OFFSET_V = 2.5;        // Datasheet: 2.5V at 0g for D03
const float SENSOR_SENSITIVITY_V_PER_G = 2.0;     // Datasheet: 2 V/g for D03 (used for arcsin conversion)

// For linear approximation (less accurate, but for reference from datasheet)
const float SENSOR_SENSITIVITY_MV_PER_DEG_LINEAR = 35.0; // 35 mV/degree for D03
const float SENSOR_SENSITIVITY_V_PER_DEG_LINEAR = SENSOR_SENSITIVITY_MV_PER_DEG_LINEAR / 1000.0;

// Calibration Parameters
const int CALIBRATION_SAMPLES_COUNT = 10;      // Number of samples for calibration
const int CALIBRATION_DELAY_MS = 20;           // Delay between calibration samples
const int CALIBRATION_STABILITY_THRESHOLD_ADC = 20; // Max ADC deviation for stable calibration

// --- Global Variables ---
// Configuration & State
int g_smoothing_window_samples = 5;         // Moving average samples count
int g_sampling_freq_hz = 10;                // Desired sampling frequency (Hz)
unsigned long g_sample_interval_ms = 100;   // Period between samples (ms), derived from g_sampling_freq_hz
float g_tilt_threshold_deg = 5.0;           // Degrees to trigger tilt alert

// Calibration Offsets (ADC values for 0-degree tilt)
float g_calib_offset_x_adc = 512.0;
float g_calib_offset_y_adc = 512.0;

// Smoothing filter buffers & sums (ADC values)
float g_x_readings_adc[MAX_SMOOTHING_SAMPLES];
float g_y_readings_adc[MAX_SMOOTHING_SAMPLES];
int g_current_read_index = 0;
float g_x_sum_adc = 0.0;
float g_y_sum_adc = 0.0;
int g_num_readings_in_filter = 0; // Current number of valid readings in the filter

// Sensor operational state
enum SensorState { SS_IDLE, SS_CALIBRATING, SS_MONITORING };
SensorState g_current_sensor_state = SS_IDLE;
unsigned long g_last_sample_time_ms = 0; // Timestamp of the last sample taken

// For test scenario TS_FORCE_CALIB_FAIL
static bool s_force_calib_fail_toggle = false; // Static to toggle values during calibration test

// --- Function Prototypes ---
void reset_smoothing_filter();
void get_raw_tilt_values_adc(int& raw_x_adc, int& raw_y_adc);
bool calibrate_sensor();
void apply_smoothing_filter(int raw_x_adc, int raw_y_adc, float& smoothed_x_adc, float& smoothed_y_adc);
void convert_adc_to_degrees(float smoothed_adc_x, float smoothed_adc_y, float& pitch_deg, float& roll_deg);
void start_sensor_monitoring(bool perform_calibration);

// --- Arduino Setup Function ---
void setup() {
  analogReadResolution(10); // Set ADC resolution to 10 bits (0-1023)

  Serial.begin(115200);
  pinMode(X_AXIS_PIN, INPUT);
  pinMode(Y_AXIS_PIN, INPUT);

  Serial.println(F("--- Tilt Sensor Demo (SCA121T-D07) ---"));

  // Calculate initial sample interval based on frequency
  g_sample_interval_ms = 1000 / g_sampling_freq_hz;

  // Test Scenario Setup
  switch (G_CURRENT_TEST_SCENARIO) {
    case TS_NORMAL_OP:
      start_sensor_monitoring(true);
      break;
    case TS_CONFIG_SMOOTHING:
      start_sensor_monitoring(true);
      if (g_current_sensor_state == SS_MONITORING) {
        delay(1000); // Delay for observation in test scenario
        Serial.println(F("CFG: Smoothing window = 3 samples"));
        g_smoothing_window_samples = 3;
        reset_smoothing_filter();
      }
      break;
    case TS_CONFIG_FREQUENCY:
      start_sensor_monitoring(true);
      if (g_current_sensor_state == SS_MONITORING) {
        delay(1000); // Delay for observation in test scenario
        Serial.println(F("CFG: Sampling Frequency = 5 Hz"));
        g_sampling_freq_hz = 5;
        g_sample_interval_ms = 1000 / g_sampling_freq_hz;
      }
      break;
    case TS_CONFIG_THRESHOLD:
      start_sensor_monitoring(true);
      if (g_current_sensor_state == SS_MONITORING) {
        delay(1000); // Delay for observation in test scenario
        Serial.println(F("CFG: Tilt Threshold = 3.0 degrees"));
        g_tilt_threshold_deg = 3.0;
      }
      break;
    case TS_RECALIBRATE_CMD:
      start_sensor_monitoring(true);
      if (g_current_sensor_state == SS_MONITORING) {
        delay(1000); // Delay for observation in test scenario
        Serial.println(F("CMD: Attempting Recalibration..."));
        if (calibrate_sensor()) {
          g_current_sensor_state = SS_MONITORING;
          g_last_sample_time_ms = millis(); // Reset sample timer
        }
      }
      break;
    case TS_STOP_CMD:
      start_sensor_monitoring(true);
      if (g_current_sensor_state == SS_MONITORING) {
        delay(1000); // Delay for observation in test scenario
        Serial.println(F("CMD: Stopping sensor monitoring."));
        g_current_sensor_state = SS_IDLE;
      }
      break;
    case TS_FORCE_CALIB_FAIL:
      start_sensor_monitoring(true); // Failure is simulated within calibrate_sensor()
      break;
    case TS_FORCE_TILT_ALERT:
      start_sensor_monitoring(true); // Alert is simulated within get_raw_tilt_values_adc()
      break;
  }
  Serial.println(F("--- Setup Complete ---"));
}

// --- Arduino Loop Function ---
void loop() {
  if (g_current_sensor_state == SS_MONITORING) {
    if (millis() - g_last_sample_time_ms >= g_sample_interval_ms) {
      g_last_sample_time_ms = millis(); // Update last sample time

      int raw_x_adc, raw_y_adc;
      float smoothed_x_adc, smoothed_y_adc;
      float pitch_deg, roll_deg;

      get_raw_tilt_values_adc(raw_x_adc, raw_y_adc);
      apply_smoothing_filter(raw_x_adc, raw_y_adc, smoothed_x_adc, smoothed_y_adc);
      convert_adc_to_degrees(smoothed_x_adc, smoothed_y_adc, pitch_deg, roll_deg);

      Serial.print(F("Pitch: ")); Serial.print(pitch_deg, 1);
      Serial.print(F(" deg, Roll: ")); Serial.print(roll_deg, 1);
      Serial.println(F(" deg"));

      if (abs(pitch_deg) > g_tilt_threshold_deg || abs(roll_deg) > g_tilt_threshold_deg) {
        Serial.print(F("!!! TILT ALERT !!! Pitch: ")); Serial.print(pitch_deg, 1);
        Serial.print(F(" deg, Roll: ")); Serial.print(roll_deg, 1);
        Serial.println(F(" deg"));
      }
    }
  }
  // No delay() here to keep the loop non-blocking; timing is handled by millis().
}

// --- Helper Function Implementations ---

/**
 * @brief Resets the smoothing filter arrays and sums.
 *        Typically called after calibration or a change in smoothing window size.
 *        Pre-fills the buffer with current calibrated zero-point ADC values.
 */
void reset_smoothing_filter() {
  g_x_sum_adc = 0.0;
  g_y_sum_adc = 0.0;
  g_num_readings_in_filter = 0;
  g_current_read_index = 0;

  // Pre-fill buffer with current zero-point (calibrated offset)
  for (int i = 0; i < MAX_SMOOTHING_SAMPLES; ++i) {
    g_x_readings_adc[i] = g_calib_offset_x_adc;
    g_y_readings_adc[i] = g_calib_offset_y_adc;
  }

  if (g_smoothing_window_samples > 0 && g_smoothing_window_samples <= MAX_SMOOTHING_SAMPLES) {
    // Calculate sum for the new window size using the pre-filled calibration values
    for (int i = 0; i < g_smoothing_window_samples; ++i) {
      g_x_sum_adc += g_x_readings_adc[i];
      g_y_sum_adc += g_y_readings_adc[i];
    }
    g_num_readings_in_filter = g_smoothing_window_samples; // Filter is now 'full'
  } else {
    g_num_readings_in_filter = 0; // Invalid smoothing window, effectively disables smoothing
    Serial.println(F("Warning: Invalid smoothing window size. Smoothing disabled."));
  }
}

/**
 * @brief Reads raw ADC values from the tilt sensor's X and Y axes.
 *        Includes logic for test scenarios to force specific readings.
 * @param raw_x_adc Reference to store the raw X-axis ADC value.
 * @param raw_y_adc Reference to store the raw Y-axis ADC value.
 */
void get_raw_tilt_values_adc(int& raw_x_adc, int& raw_y_adc) {
  if (G_CURRENT_TEST_SCENARIO == TS_FORCE_TILT_ALERT && g_current_sensor_state == SS_MONITORING) {
    // Force a tilt alert by creating an ADC deviation corresponding to threshold + 2 degrees
    float adc_deviation_for_alert = (g_tilt_threshold_deg + 2.0) / ((VCC_VOLTAGE_V / ADC_MAX_VALUE) * DEGREES_PER_VOLT);
    raw_x_adc = g_calib_offset_x_adc + (int)adc_deviation_for_alert;
    raw_y_adc = g_calib_offset_y_adc; // Keep Y-axis stable for simplicity
  } else if (G_CURRENT_TEST_SCENARIO == TS_FORCE_CALIB_FAIL && g_current_sensor_state == SS_CALIBRATING) {
    // Force unstable readings during calibration test
    int instability_adc = CALIBRATION_STABILITY_THRESHOLD_ADC + 5; // Ensure it's above threshold
    raw_x_adc = 512 + (s_force_calib_fail_toggle ? instability_adc : -instability_adc);
    raw_y_adc = 512 + (s_force_calib_fail_toggle ? instability_adc : -instability_adc);
    s_force_calib_fail_toggle = !s_force_calib_fail_toggle; // Toggle for varying readings
  } else {
    raw_x_adc = analogRead(X_AXIS_PIN);
    raw_y_adc = analogRead(Y_AXIS_PIN);
  }
}

/**
 * @brief Calibrates the sensor by reading multiple samples to determine the zero-tilt offset.
 *        Sets the current sensor position as the zero-degree reference.
 * @return true if calibration was successful (readings were stable), false otherwise.
 */
bool calibrate_sensor() {
  Serial.println(F("Sensor CALIBRATING..."));
  g_current_sensor_state = SS_CALIBRATING;

  long sum_x_adc = 0, sum_y_adc = 0;
  int min_x_adc = 1024, max_x_adc = 0;
  int min_y_adc = 1024, max_y_adc = 0;
  int current_raw_x_adc, current_raw_y_adc;

  for (int i = 0; i < CALIBRATION_SAMPLES_COUNT; i++) {
    get_raw_tilt_values_adc(current_raw_x_adc, current_raw_y_adc);
    sum_x_adc += current_raw_x_adc;
    sum_y_adc += current_raw_y_adc;

    if (current_raw_x_adc < min_x_adc) min_x_adc = current_raw_x_adc;
    if (current_raw_x_adc > max_x_adc) max_x_adc = current_raw_x_adc;
    if (current_raw_y_adc < min_y_adc) min_y_adc = current_raw_y_adc;
    if (current_raw_y_adc > max_y_adc) max_y_adc = current_raw_y_adc;

    delay(CALIBRATION_DELAY_MS); // Use delay here as calibration is a distinct, blocking phase
  }

  if ((max_x_adc - min_x_adc > CALIBRATION_STABILITY_THRESHOLD_ADC) ||
      (max_y_adc - min_y_adc > CALIBRATION_STABILITY_THRESHOLD_ADC)) {
    Serial.println(F("Calibration FAILED: Readings too unstable."));
    g_current_sensor_state = SS_IDLE;
    return false;
  }

  g_calib_offset_x_adc = (float)sum_x_adc / CALIBRATION_SAMPLES_COUNT;
  g_calib_offset_y_adc = (float)sum_y_adc / CALIBRATION_SAMPLES_COUNT;

  Serial.print(F("Calibration OK. Offset X_ADC: ")); Serial.print(g_calib_offset_x_adc);
  Serial.print(F(" Y_ADC: ")); Serial.println(g_calib_offset_y_adc);

  reset_smoothing_filter(); // Reset filter with new calibration data
  return true;
}

/**
 * @brief Applies a moving average filter to the raw ADC sensor readings.
 * @param raw_x_adc The raw X-axis ADC value.
 * @param raw_y_adc The raw Y-axis ADC value.
 * @param smoothed_x_adc Reference to store the smoothed X-axis ADC value.
 * @param smoothed_y_adc Reference to store the smoothed Y-axis ADC value.
 */
void apply_smoothing_filter(int raw_x_adc, int raw_y_adc, float& smoothed_x_adc, float& smoothed_y_adc) {
  // If smoothing window is invalid or filter isn't initialized (e.g., too few readings for full window)
  if (g_smoothing_window_samples <= 0 || g_smoothing_window_samples > MAX_SMOOTHING_SAMPLES || g_num_readings_in_filter < g_smoothing_window_samples) {
    // If filter isn't 'full' yet, but we have some readings, this logic effectively uses fewer samples until full.
    // However, reset_smoothing_filter pre-fills, so numReadingsInFilter should be smoothingWindowSamples or 0.
    // If numReadingsInFilter is 0 due to invalid window size, pass raw values.
    if (g_num_readings_in_filter == 0) {
        smoothed_x_adc = raw_x_adc;
        smoothed_y_adc = raw_y_adc;
        return;
    }
  }

  // Standard moving average: subtract the oldest reading, add the newest
  g_x_sum_adc -= g_x_readings_adc[g_current_read_index];
  g_y_sum_adc -= g_y_readings_adc[g_current_read_index];

  g_x_readings_adc[g_current_read_index] = raw_x_adc;
  g_y_readings_adc[g_current_read_index] = raw_y_adc;

  g_x_sum_adc += raw_x_adc;
  g_y_sum_adc += raw_y_adc;

  g_current_read_index = (g_current_read_index + 1) % g_smoothing_window_samples;

  smoothed_x_adc = g_x_sum_adc / g_smoothing_window_samples;
  smoothed_y_adc = g_y_sum_adc / g_smoothing_window_samples;
}

/**
 * @brief Converts smoothed ADC values to tilt angles (pitch and roll) in degrees
 *        using the arcsin method recommended by the SCA121T datasheet for accuracy.
 * @param smoothed_adc_x The smoothed X-axis ADC value.
 * @param smoothed_adc_y The smoothed Y-axis ADC value.
 * @param pitch_deg Reference to store the calculated pitch in degrees.
 * @param roll_deg Reference to store the calculated roll in degrees.
 */
void convert_adc_to_degrees(float smoothed_adc_x, float smoothed_adc_y, float& pitch_deg, float& roll_deg) {
    // Calculate the voltage deviation from the *calibrated* zero-g point for each axis
    // g_calib_offset_x_adc is the ADC reading that our calibration determined to be 0 degrees
    // for the current (5V) supply.
    float delta_voltage_x_v = (smoothed_adc_x - g_calib_offset_x_adc) * (VCC_VOLTAGE_V / ADC_MAX_VALUE);
    float delta_voltage_y_v = (smoothed_adc_y - g_calib_offset_y_adc) * (VCC_VOLTAGE_V / ADC_MAX_VALUE);

    // The argument for arcsin is (Delta_V / Sensitivity_in_V_per_g).
    // This gives sin(angle), because 1g corresponds to the full sensitivity voltage swing.
    float sin_pitch = delta_voltage_x_v / SENSOR_SENSITIVITY_V_PER_G;
    float sin_roll  = delta_voltage_y_v / SENSOR_SENSITIVITY_V_PER_G;

    // Clamp the arguments to asin to the valid range [-1.0, 1.0] to prevent NaN errors
    // This can happen due to noise or if the sensor/calibration is slightly off at extremes.
    if (sin_pitch > 1.0) sin_pitch = 1.0;
    if (sin_pitch < -1.0) sin_pitch = -1.0;
    if (sin_roll > 1.0) sin_roll = 1.0;
    if (sin_roll < -1.0) sin_roll = -1.0;

    // Calculate angle in radians using arcsin, then convert to degrees
    pitch_deg = degrees(asin(sin_pitch));
    roll_deg  = degrees(asin(sin_roll));
}

/**
 * @brief Initiates sensor monitoring.
 * @param perform_calibration If true, sensor calibration will be performed before monitoring starts.
 *                            If false, existing calibration offsets are used.
 */
void start_sensor_monitoring(bool perform_calibration) {
  Serial.println(F("CMD: Attempting to start sensor monitoring..."));
  if (perform_calibration) {
    if (!calibrate_sensor()) {
      Serial.println(F("Start FAILED (due to calibration failure). Sensor remains IDLE."));
      // g_current_sensor_state is already set to SS_IDLE by calibrate_sensor() on failure
      return;
    }
  } else {
    // If not calibrating, ensure the filter is reset using existing/default offsets
    Serial.println(F("INFO: Skipping calibration, using existing/default offsets."));
    reset_smoothing_filter();
  }

  Serial.println(F("Sensor monitoring started. State: MONITORING."));
  g_current_sensor_state = SS_MONITORING;
  g_last_sample_time_ms = millis(); // Initialize sample timer
}
