//*****************************************************************************
// * File: tilt_sensor.ino
// * Author: Lily Trautman
// * Created: May 16 2025
// * Modified: May 31 2025
// *
// * Brief: Implements the full functionality of the SCA121T-D03 tilt sensor
// *        module for a vault security system. Handles configuration,
// *        commands, data reporting, and alerts via JSON messaging.
// *
// * Hardware: SCA121T-D03 Tilt Sensor
// *   - Red:    +5V
// *   - Blue:   GND
// *   - Yellow: X-axis output -> Arduino A0
// *   - Green:  Y-axis output -> Arduino A1
// *****************************************************************************/

#include <ArduinoJson.h>
#include <vector> //for average calculation
#include <cmath> //for average calculation

// --- Module Identification ---
const char* MODULE_ID = "tilt_sensor1";

// --- Constant Definitions (Remain Global or become static const members if preferred) ---
// Pin Assignments
const int X_AXIS_PIN = A0; // Yellow wire
const int Y_AXIS_PIN = A1; // Green wire
// JSON Buffer Sizes (Primarily for messaging, can remain global)
const int JSON_RX_BUFFER_SIZE = 256;
const int JSON_TX_BUFFER_SIZE = 256;
// Sensor Hardware/System Constants
const int MAX_SMOOTHING_SAMPLES = 20;
const float ADC_MAX_VALUE = 4095.0;
const float VCC_VOLTAGE_V = 5.0;
const float SENSOR_SENSITIVITY_V_PER_G = 2.0;
const int CALIBRATION_SAMPLES_COUNT = 100;
const int CALIBRATION_STABILITY_THRESHOLD_ADC = 20;

void send_error_message(const char* to_id, const char* error_type, const char* reason);
void send_alert_message(const char* event, float pitch, float roll, bool pitch_triggered, bool roll_triggered);
void send_status_message(const char* to_id, const char* status, const char* details_name = nullptr, const char* details_value = nullptr);
void send_config_response(const char* to_id, const char* param_name, float value, bool success, const char* message_detail);

class TiltSensor {
public:
    // Publicly accessible enum for state, scoped to the class
    enum SensorStateEnum { SS_IDLE, SS_CALIBRATING, SS_MONITORING };

    // --- Configuration Members (formerly g_ variables) ---
    int smoothing_window_samples;
    int sampling_freq_hz;
    float tilt_threshold_deg;

    // --- Derived State & Calibration Members ---
    unsigned long sample_interval_ms;
    float calib_offset_x_adc;
    float calib_offset_y_adc;

    // --- Operational State Members ---
    SensorStateEnum current_sensor_state;
    bool monitoring_active;
    unsigned long last_sample_time_ms;
    bool alert_sent_for_current_tilt;

    // --- Smoothing Filter Data (Internal to sensor operation) ---
private: 
    float x_readings_adc[MAX_SMOOTHING_SAMPLES];
    float y_readings_adc[MAX_SMOOTHING_SAMPLES];
    int current_read_index;
    float x_sum_adc;
    float y_sum_adc;
    int num_readings_in_filter;

public:
    TiltSensor() :
        // Initialize members in the constructor
        smoothing_window_samples(5),
        sampling_freq_hz(10),
        tilt_threshold_deg(5.0f),
        calib_offset_x_adc(ADC_MAX_VALUE / 2.0f),
        calib_offset_y_adc(ADC_MAX_VALUE / 2.0f),
        current_sensor_state(SS_IDLE),
        monitoring_active(false),
        last_sample_time_ms(0),
        alert_sent_for_current_tilt(false),
        current_read_index(0),
        x_sum_adc(0.0f),
        y_sum_adc(0.0f),
        num_readings_in_filter(0)
    {
        update_sample_interval(); // Calculate initial sample_interval_ms
    }

    // --- Public Methods ---
    void init(); // For one-time setup calculations for the sensor object
    void update_sample_interval(); // Call when sampling_freq_hz changes

    void reset_smoothing_filter();
    void get_raw_tilt_values_adc(int& raw_x_adc, int& raw_y_adc) const; // const as it doesn't modify sensor state directly
    bool calibrate_sensor(const char* requester_id);
    void apply_smoothing_filter(int raw_x_adc, int raw_y_adc, float& smoothed_x_adc, float& smoothed_y_adc);
    void convert_adc_to_degrees(float smoothed_adc_x, float smoothed_adc_y, float& pitch_deg, float& roll_deg) const; // const
    
    bool start_monitoring(bool perform_calibration, const char* requester_id);
    void stop_monitoring(const char* requester_id);
    
    void process_monitoring_tick(); // Handles the periodic sensor reading and alerting
};

// --- Global Instance of the Sensor ---
TiltSensor sensor;

// --- Method Implementations for TiltSensor ---
void TiltSensor::init() {
    update_sample_interval(); // Ensure interval is correct based on initial frequency
    reset_smoothing_filter();
}

void TiltSensor::update_sample_interval() {
    if (sampling_freq_hz > 0) {
        sample_interval_ms = 1000 / sampling_freq_hz;
    } else {
        sampling_freq_hz = 1; // Safety: prevent division by zero, 1Hz min
        sample_interval_ms = 1000;
    }
}

void TiltSensor::reset_smoothing_filter() {
    x_sum_adc = 0.0f;
    y_sum_adc = 0.0f;
    num_readings_in_filter = 0;
    current_read_index = 0;

    // Pre-fill buffer with current zero-point (calibrated offset)
    for (int i = 0; i < MAX_SMOOTHING_SAMPLES; ++i) {
        x_readings_adc[i] = calib_offset_x_adc;
        y_readings_adc[i] = calib_offset_y_adc;
    }

    if (smoothing_window_samples > 0 && smoothing_window_samples <= MAX_SMOOTHING_SAMPLES) {
        for (int i = 0; i < smoothing_window_samples; ++i) {
            x_sum_adc += x_readings_adc[i];
            y_sum_adc += y_readings_adc[i];
        }
        num_readings_in_filter = smoothing_window_samples; // Filter is now 'full'
    } else {
        smoothing_window_samples = 1; 
        x_sum_adc = x_readings_adc[0];
        y_sum_adc = y_readings_adc[0];
        num_readings_in_filter = 1;
    }
}

void TiltSensor::get_raw_tilt_values_adc(int& raw_x_adc, int& raw_y_adc) const {
    raw_x_adc = analogRead(X_AXIS_PIN);
    raw_y_adc = analogRead(Y_AXIS_PIN);
}

bool TiltSensor::calibrate_sensor(const char* requester_id) {
    SensorStateEnum prev_state = current_sensor_state;
    current_sensor_state = SS_CALIBRATING;

    std::vector<int> x_samples(CALIBRATION_SAMPLES_COUNT);
    std::vector<int> y_samples(CALIBRATION_SAMPLES_COUNT);
    long temp_sum_x_adc = 0;
    long temp_sum_y_adc = 0;

    for (int i = 0; i < CALIBRATION_SAMPLES_COUNT; i++) {
        get_raw_tilt_values_adc(x_samples[i], y_samples[i]);
        temp_sum_x_adc += x_samples[i];
        temp_sum_y_adc += y_samples[i];
        // delay(1); //Might need this for overscan debounce
    }

    bool x_stable = (max_x_adc - min_x_adc <= CALIBRATION_STABILITY_THRESHOLD_ADC);
    bool y_stable = (max_y_adc - min_y_adc <= CALIBRATION_STABILITY_THRESHOLD_ADC);

    if (!x_stable || !y_stable) {
        Serial.println(F("Calibration FAILED: Readings too unstable."));
        String reason = "Unstable readings. X_delta: " + String(max_x_adc - min_x_adc) +
                        ", Y_delta: " + String(max_y_adc - min_y_adc);
        // Calls the global send_error_message function
        ::send_error_message(requester_id, "calibration_failed", reason.c_str()); 
        current_sensor_state = SS_IDLE;
        return false;
    }

    calib_offset_x_adc = avg_x_adc;
    calib_offset_y_adc = avg_y_adc;

    reset_smoothing_filter();
    current_sensor_state = (prev_state == SS_CALIBRATING ? SS_IDLE : prev_state);
    if (current_sensor_state == SS_IDLE && monitoring_active) {
        current_sensor_state = SS_MONITORING;
    }
    return true;
}

void TiltSensor::apply_smoothing_filter(int raw_x_adc, int raw_y_adc, float& smoothed_x_adc, float& smoothed_y_adc) {
    if (smoothing_window_samples <= 0 || smoothing_window_samples > MAX_SMOOTHING_SAMPLES) {
        smoothed_x_adc = raw_x_adc;
        smoothed_y_adc = raw_y_adc;
        return;
    }

    if (num_readings_in_filter < smoothing_window_samples) {
        x_readings_adc[num_readings_in_filter] = raw_x_adc;
        y_readings_adc[num_readings_in_filter] = raw_y_adc;
        x_sum_adc += raw_x_adc;
        y_sum_adc += raw_y_adc;
        num_readings_in_filter++;
        smoothed_x_adc = x_sum_adc / num_readings_in_filter;
        smoothed_y_adc = y_sum_adc / num_readings_in_filter;
        current_read_index = num_readings_in_filter % smoothing_window_samples;
        return;
    }

    x_sum_adc -= x_readings_adc[current_read_index];
    y_sum_adc -= y_readings_adc[current_read_index];
    x_readings_adc[current_read_index] = raw_x_adc;
    y_readings_adc[current_read_index] = raw_y_adc;
    x_sum_adc += raw_x_adc;
    y_sum_adc += raw_y_adc;
    current_read_index = (current_read_index + 1) % smoothing_window_samples;
    smoothed_x_adc = x_sum_adc / smoothing_window_samples;
    smoothed_y_adc = y_sum_adc / smoothing_window_samples;
}

void TiltSensor::convert_adc_to_degrees(float smoothed_adc_x, float smoothed_adc_y, float& pitch_deg, float& roll_deg) const {
    float delta_voltage_x_v = (smoothed_adc_x - calib_offset_x_adc) * (VCC_VOLTAGE_V / ADC_MAX_VALUE);
    float delta_voltage_y_v = (smoothed_adc_y - calib_offset_y_adc) * (VCC_VOLTAGE_V / ADC_MAX_VALUE);
    float sin_pitch = constrain(delta_voltage_x_v / SENSOR_SENSITIVITY_V_PER_G, -1.0f, 1.0f);
    float sin_roll  = constrain(delta_voltage_y_v / SENSOR_SENSITIVITY_V_PER_G, -1.0f, 1.0f);
    pitch_deg = degrees(asin(sin_pitch));
    roll_deg  = degrees(asin(sin_roll));
}

bool TiltSensor::start_monitoring(bool perform_calibration, const char* requester_id) {
    bool was_monitoring_locally = monitoring_active; // Use local var for temp check

    if (perform_calibration) {
        if (!calibrate_sensor(requester_id)) { // Call member function
            current_sensor_state = SS_IDLE;
            monitoring_active = false;
            return false;
        }
    } else if (current_sensor_state != SS_MONITORING && calib_offset_x_adc == (ADC_MAX_VALUE / 2.0f)) {
        if (!calibrate_sensor(requester_id)) { // Call member function
            current_sensor_state = SS_IDLE;
            monitoring_active = false;
            return false;
        }
    }
  
    if (!perform_calibration) {
        reset_smoothing_filter(); // Call member function
    }

    current_sensor_state = SS_MONITORING;
    monitoring_active = true;
    last_sample_time_ms = millis();
    alert_sent_for_current_tilt = false;

    return true;
}

void TiltSensor::stop_monitoring(const char* requester_id) {
    if (monitoring_active) {
    }
    monitoring_active = false;
    current_sensor_state = SS_IDLE;
}

void TiltSensor::process_monitoring_tick() {
    if (monitoring_active && current_sensor_state == SS_MONITORING) {
        if (millis() - last_sample_time_ms >= sample_interval_ms) {
            last_sample_time_ms = millis();

            int raw_x_adc, raw_y_adc;
            float smoothed_x_adc, smoothed_y_adc;
            float pitch_deg, roll_deg;

            get_raw_tilt_values_adc(raw_x_adc, raw_y_adc);
            apply_smoothing_filter(raw_x_adc, raw_y_adc, smoothed_x_adc, smoothed_y_adc);
            convert_adc_to_degrees(smoothed_x_adc, smoothed_y_adc, pitch_deg, roll_deg);

            ::send_data_message(pitch_deg, roll_deg);

            bool pitch_exceeded = abs(pitch_deg) > tilt_threshold_deg;
            bool roll_exceeded = abs(roll_deg) > tilt_threshold_deg;

            if (pitch_exceeded || roll_exceeded) {
                if (!alert_sent_for_current_tilt) {
                    ::send_alert_message("tilt_threshold_exceeded", pitch_deg, roll_deg, pitch_exceeded, roll_exceeded);
                    alert_sent_for_current_tilt = true;
                }
            } else {
                alert_sent_for_current_tilt = false;
            }
        }
    }
}

// --- Global Messaging and Configuration Functions ---
void on_sensor_config_number_sample_changed() {
    sensor.reset_smoothing_filter();
}

void on_sensor_config_frequency_hz_changed() {
    sensor.update_sample_interval();
}

bool process_int_config_param(const JsonDocument& doc, const char* from_id, const char* param_name,
                              int& target_variable, int min_val, int max_val,
                              const char* range_error_detail, void (*success_callback)() = nullptr) {
    if (!doc.containsKey("value") || !doc["value"].is<int>()) {
        ::send_config_response(from_id, param_name, 0, false, "invalid_or_missing_value_int");
        return false;
    }
    int val = doc["value"].as<int>();
    if (val >= min_val && val <= max_val) {
        target_variable = val; // This will modify sensor.smoothing_window_samples directly
        if (success_callback) {
            success_callback();
        }
        ::send_config_response(from_id, param_name, static_cast<float>(val), true, "applied");
        return true;
    } else {
        ::send_config_response(from_id, param_name, static_cast<float>(val), false, range_error_detail);
        return false;
    }
}

bool process_float_config_param(const JsonDocument& doc, const char* from_id, const char* param_name,
                                float& target_variable, float min_val, float max_val,
                                const char* range_error_detail, void (*success_callback)() = nullptr) {
    if (!doc.containsKey("value") || (!doc["value"].is<float>() && !doc["value"].is<int>())) {
        ::send_config_response(from_id, param_name, 0, false, "invalid_or_missing_value_float_or_int");
        return false;
    }
    float val = doc["value"].as<float>();
    if (val >= min_val && val <= max_val) {
        target_variable = val; // This will modify sensor.tilt_threshold_deg directly
        if (success_callback) {
            success_callback();
        }
        ::send_config_response(from_id, param_name, val, true, "applied");
        return true;
    } else {
        ::send_config_response(from_id, param_name, val, false, range_error_detail);
        return false;
    }
}

void handle_config_message(const JsonDocument& doc, const char* from_id) {
    if (!doc.containsKey("name") || !doc["name"].is<const char*>()) {
        ::send_error_message(from_id, "invalid_config", "config 'name' field missing or not a string");
        return;
    }

    const char* name = doc["name"].as<const char*>();

    if (name == nullptr || name[0] == '\0') {
        ::send_error_message(from_id, "invalid_config", "config 'name' field is empty");
        return;
    }

    if (strcmp(name, "number_sample") == 0) {
        process_int_config_param(doc, from_id, name, sensor.smoothing_window_samples, 1, MAX_SMOOTHING_SAMPLES,
                                 "value_out_of_range", on_sensor_config_number_sample_changed);
    } else if (strcmp(name, "frequency_hz") == 0) {
        process_int_config_param(doc, from_id, name, sensor.sampling_freq_hz, 1, 100,
                                 "value_out_of_range (1-100)", on_sensor_config_frequency_hz_changed);
    } else if (strcmp(name, "threshold") == 0) {
        process_float_config_param(doc, from_id, name, sensor.tilt_threshold_deg, 0.1f, 90.0f,
                                   "value_out_of_range (0.1-90.0)"); // No callback needed
    } else if (strcmp(name, "calibrate") == 0) {
        bool prev_monitoring_state = sensor.monitoring_active;
        if (sensor.monitoring_active) {
            sensor.stop_monitoring(from_id); // Use sensor object method
        }
        
        if (sensor.calibrate_sensor(from_id)) { // Use sensor object method
            ::send_config_response(from_id, name, 0, true, "calibration_successful");
            if (prev_monitoring_state) {
                sensor.start_monitoring(false, from_id); // Use sensor object method
            } else {
                sensor.current_sensor_state = TiltSensor::SS_IDLE; 
            }
        } else {
            // calibrate_sensor already sent a detailed error
            ::send_config_response(from_id, name, 0, false, "calibration_failed_see_error_log");
            sensor.current_sensor_state = TiltSensor::SS_IDLE; 
        }
    } else {
        ::send_error_message(from_id, "unknown_config_name", name);
    }
}

void handle_json_message(const String& json_string) {
    StaticJsonDocument<JSON_RX_BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, json_string);

    if (error) {
        ::send_error_message("*", "malformed_json", error.c_str());
        return;
    }

    bool mtype_present = doc.containsKey("mtype");
    bool from_present = doc.containsKey("from");
    bool to_present = doc.containsKey("to");

    bool mtype_is_string = mtype_present && doc["mtype"].is<const char*>();
    bool from_is_string = from_present && doc["from"].is<const char*>();
    bool to_is_string = to_present && doc["to"].is<const char*>();

    if (!mtype_is_string || !from_is_string || !to_is_string) {
        String error_detail = "Invalid request structure:";
        if (!mtype_present) error_detail += " 'mtype' field missing.";
        else if (!mtype_is_string) error_detail += " 'mtype' field must be a string.";
        
        if (!from_present) error_detail += " 'from' field missing.";
        else if (!from_is_string) error_detail += " 'from' field must be a string.";

        if (!to_present) error_detail += " 'to' field missing.";
        else if (!to_is_string) error_detail += " 'to' field must be a string.";

        const char* recipient_for_error = "*";
        if (from_is_string) {
            const char* temp_from_val = doc["from"].as<const char*>();
            if (temp_from_val && temp_from_val[0] != '\0') { 
                recipient_for_error = temp_from_val;
            }
        }
        ::send_error_message(recipient_for_error, "invalid_request_structure", error_detail.c_str());
       return;
    }

    const char* mtype_value = doc["mtype"].as<const char*>();
    const char* from_value = doc["from"].as<const char*>();
    const char* to_value = doc["to"].as<const char*>();

    if (strcmp(to_value, MODULE_ID) != 0 && strcmp(to_value, "*") != 0) {
        return; // Message not for this module, silently ignore.
    }

    if (from_value[0] == '\0' || strcmp(from_value, "NOT_VALID") == 0) {
        return; 
    }

    if (mtype_value[0] == '\0') {
        ::send_error_message(from_value, "invalid_mtype", "mtype_is_empty");
        return;
    }

    if (strcmp(mtype_value, "config") == 0) {
        handle_config_message(doc, from_value);
    } else if (strcmp(mtype_value, "tilt_sensor.start") == 0) {
       if (sensor.monitoring_active && sensor.current_sensor_state == TiltSensor::SS_MONITORING) {
             ::send_status_message(from_value, "command_ack", "tilt_sensor.start", "already_monitoring");
        } else if (sensor.start_monitoring(false, from_value)) {

            ::send_status_message(from_value, "command_ack", "tilt_sensor.start", "monitoring_started");
       }

    } else if (strcmp(mtype_value, "tilt_sensor.stop") == 0) {
        sensor.stop_monitoring(from_value);
        ::send_status_message(from_value, "command_ack", "tilt_sensor.stop", "monitoring_stopped");
    } else {
        String reason = "Unknown mtype value: '";
        reason += mtype_value;
        reason += "'";
        ::send_error_message(from_value, "unknown_mtype", reason.c_str());
   }
}

// --- Arduino Setup Function ---
void setup() {
    analogReadResolution(12);
    Serial.begin(115200);
    while (!Serial && millis() < 5000);
    
    pinMode(X_AXIS_PIN, INPUT);
    pinMode(Y_AXIS_PIN, INPUT);

    sensor.init(); // Initialize the sensor object's internal state
}

// --- Arduino Loop Function ---
void loop() {
    if (Serial.available() > 0) {
        String input_line = Serial.readStringUntil('\n');
        input_line.trim();
        if (input_line.length() > 0) {
            handle_json_message(input_line);
        }
    }
    sensor.process_monitoring_tick(); // Let the sensor object handle its periodic tasks
}

// --- JSON Message Sending Functions (Implementations) ---
void send_json_reply(const JsonDocument& doc) {
  String output_json;
  serializeJson(doc, output_json);
  Serial.println(output_json);
}

void send_alert_message(const char* event, float pitch, float roll, bool pitch_triggered, bool roll_triggered) {
  StaticJsonDocument<JSON_TX_BUFFER_SIZE> doc;
  doc["mtype"] = String(MODULE_ID) + ".alert"; 
  doc["from"] = MODULE_ID;
  doc["to"] = "*";
  doc["event"] = event;
  if (pitch_triggered) doc["pitch"] = serialized(String(pitch, 1));
  if (roll_triggered) doc["roll"] = serialized(String(roll, 1));
  if (!pitch_triggered && !roll_triggered && (pitch != 0.0 || roll != 0.0)) {
     doc["pitch"] = serialized(String(pitch, 1));
     doc["roll"] = serialized(String(roll, 1));
  }
  send_json_reply(doc);
}

void send_error_message(const char* to_id, const char* error_type, const char* reason) {
  StaticJsonDocument<JSON_TX_BUFFER_SIZE> doc;
  doc["mtype"] = "error";
  doc["from"] = MODULE_ID;
  doc["to"] = to_id;
  String full_message = error_type;
  if (reason && strlen(reason) > 0) {
    full_message += ": ";
    full_message += reason;
  }
  doc["message"] = full_message;
  send_json_reply(doc);
}

void send_status_message(const char* to_id, const char* status, const char* details_name, const char* details_value) {
  StaticJsonDocument<JSON_TX_BUFFER_SIZE> doc;
  doc["mtype"] = "debug";
  doc["from"] = MODULE_ID;
  doc["to"] = to_id;
  doc["status"] = status;
  if (details_name && details_value) {
    doc.createNestedObject("details")[details_name] = details_value;
  }
  send_json_reply(doc);
}

void send_config_response(const char* to_id, const char* param_name, float value, bool success, const char* message_detail) {
  StaticJsonDocument<JSON_TX_BUFFER_SIZE> doc;
  doc["from"] = MODULE_ID;
  doc["to"] = to_id;
  if (success) {
    doc["mtype"] = String(MODULE_ID) + ".status";
    doc["status"] = "config_updated";
    JsonObject param = doc.createNestedObject("parameter");
    param["name"] = param_name;
    if (floor(value) == value && param_name && (strcmp(param_name, "number_sample")==0 || strcmp(param_name, "frequency_hz")==0) ) {
        param["value"] = (int)value; 
    } else {
        param["value"] = value; 
    }
    if (message_detail && strlen(message_detail) > 0) doc["message"] = message_detail;
  } else {
    doc["mtype"] = "error"; 
    String full_error_message = "config_failed: parameter '";
    full_error_message += param_name;
    full_error_message += "'";
    if (message_detail && strlen(message_detail) > 0) {
      full_error_message += " - reason: ";
      full_error_message += message_detail;
    }
    doc["message"] = full_error_message;
  }
  send_json_reply(doc);
}