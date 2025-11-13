  # Vibration Sensor

## Configuration Parameters

```json
{"mtype": "config", "from": "brain", "to": "vibration_sensor", "name": "baseline_vibration", "value": 0 }
```
- Sets baseline vibration of environment. Ranges from 1-100.
```json
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "sample_rate", "value": 10}
```

- Sets the rate at which the sensor takes samples. Cannot be 0.

```json
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "threshold", "value": 3}
```

- Sets threshold of what is considered a positive vibration reading. The number of positive reads out of 100 that counts as a positive vibration reading (1-100).

```json
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "broadcast_rate", "value": 1}
```
- Determines the rate at which sensor state messages are sent out when in RUNNING state. Int 1-10 Hz.

```json
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "smoothing", "value": 3}
```

- Dictates that the last x values should be taken as a running average to compare against threshold and baseline.


## Input

```json
{"mtype": "vibration_sensor.start", "from": "brain", "to": "vibration_sensor1"}
```
```json
{"mtype": "vibration_sensor.stop", "from": "brain", "to": "vibration_sensor1"}
```
```json
{"mtype": "vibration_sensor.alert", "from": "brain", "to": "vibration_sensor1"}
```
- Allows brain to manually change the state to STOP, RUNNING or ALERT.

## Output

```json
{
  "mtype": "error",
  "from": "vibration_sensor1",
  "to": "*",
  "message": "error message no newlines."
}
```

- Outputs error messages when JSON messages are invalid.

```json
{
  "mtype": "debug",
  "from": "vibration_sensor1",
  "to": "*",
  "message": "debug message no newlines."
}
```
- Outputs a debug message when config parameters or the state is changed
```json
{
  "mtype": "vibration_sensor.output",
  "from": "vibration_sensor1",
  "to": "brain",
  "name": "is_vibrating",
  "value": "boolean"
}
```

- Outputs smoothed value; running average of past x samples.

```json
{
  "mtype": "vibration_sensor.alert_output",
  "from": "vibration_sensor",
  "to": "brain",
  "name": "is_vibrating",
  "value": "boolean"
}
```

- Outputs smoothed value; running average of past x samples, alongside a string noting the current state.



## Hardware Connections

Tutorial: [SunFounder Vibration Sensor Guide](https://docs.sunfounder.com/projects/umsk/en/latest/05_raspberry_pi/pi_lesson24_vibration_sensor.html)

- Wire 5V to the positive (+) track and ground to the negative (–).
- Connect 5V to VCC and GND to ground.
- Connect D0 to DX pin on the controller.

### Hello World Sketch Hardware and Connectivity

- The Vibration Switch will output digital signals to the ESP32. LOW indicates there is a vibration detected, and HIGH means there isn't.
- Connect 5V and GND on the ESP32 to the positive and negative rails respectively.
- Connect the 5V and GND wires from the Sunfounder Vibration Switch to the 5V and GND rails on the breadboard.
- Connect the SIG wire to pin D7 on the ESP32
- The hello.ino sketch demonstrates basic functionality of the ESP32 and Vibration Switch. The ESP32 will constantly be reading the digital signal from the SIG pin on the vibration sensor. According to what state it's in, it will do nothing(STOP state), broadcast only if it detects vibration (ALERT state), or continuously broadcast the state of the sensor (RUNNING state).
- The main issue that I encountered was the fact that the vibration threshold of the sensor cannot be changed due to the nature of the sensor. The sensor uses a spring which completes a circuit when it's vibrated enough and outputs a LOW signal.
- Note that the vibration sensor must be shaken violently to detect vibration.

## Summary of Changes Made to Documentation & HW Limitations
- Added value ranges for config parameters such as smoothing and threshold.
- Changed description of states based on design decisions made during implementation.
- The vibration sensor has a high threshold for detecting vibration. Although it's rare to get a false positive, threshold and baseline vibration variables are used to smooth the data.


## Software Design

- Global variables of `g_baseline`, `g_smoothing`, and `g_threshold`
- Other variables needed for storing output data and smoothing operations.
- Sensor should Sample data, Smooth data, then Output smoothed data to the brain

### State Design Pattern

- **STOP** state: starts here, and remains until told to run by the brain through the .state input message. Returns here the same way.
- **RUNNING** state: continously broadcast sensor readings.
- **ALERT** state: only broadcast when vibration is detected.
- Additional functions can be added as needed

Reference: [SW-420 Vibration Sensor Breakdown](https://lonelybinary.com/en-us/blogs/learn/ss-vibration-sw420)

## Redis Implementation

#### Redis Hashes
- Redis stores record types as field-value pairs.
- ```HSET``` and ```HGET``` sets and retrieves fields (HSET sets multiple)
- ```HMGET``` returns one or more values of given fields
- You can also set expiration times for fields using ```HEXPIRE```. This is useful for counting number of some events over a given time period.

#### Subscribe/Unsubscribe
- ```SUBSCRIBE```, ```UNSUBSCRIBE```, ```PUBLISH``` are used for the redis messaging paradigm
- Instead of sending messages to a specific client, the publisher sends them to channels where clients are subscribed to (ex: ```SUBSCRIBE channel11 ch:00```)

#### ```load_config.py``` & Redis Server Setup
- Installed and configured redis server
- Ran redis server and monitored it through ```docker exec -it redis-server redis-cli monitor``` command
- Created a config file that holds default config variables for ```vibration_sensor```
- Ran the command ```python util/load_config.py embedded/[module_name]/scenario1_config.json```
- Received the following output:  
INFO: hostname: localhost  
INFO: port: 6379  
INFO: config file: embedded/vibration_sensor/scenario1_config.json  
INFO: connected to REDIS server at localhost:6379 successfully  
INFO: module name is: vibration_sensor1  
INFO: validate config for 'config:vibration_sensor1' :
```{'baseline_vibration': '0', 'sample_rate': '10', 'threshold': '3', 'smoothing': '3'}```

## Test Cases

### Configuration Tests
**T-CONFIG-01**: Verifies that `sample_rate` can be changed to 10 Hz.
- Input Message: 
```json 
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "sample_rate", 
  "value": 10
}
```
- Output Message: 
```json
{
  "mtype": "debug", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Successfully changed sample_rate to 10"
}
```

**T-CONFIG-02**: Verifies that `threshold` can be set to 30%.
- Input Message:
```json
{
"mtype": "config", 
"from": "brain", 
"to": "vibration_sensor1", 
"name": "threshold", 
"value": 30
}
```
- Output Message:
```json
{
  "mtype": "debug", 
"to": "*", 
"from": "vibration_sensor1", 
"message": "Successfully changed threshold to 30"
}
```
**T-CONFIG-03**: Sets `smoothing` to 15.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "smoothing", 
  "value": 15
}
```
- Output Message:
```json
{
  {"mtype": "debug", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Successfully changed smoothing to 15"}
}
```

**T-CONFIG-04**: Sets `baseline_vibration` to 40.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "baseline_vibration", 
  "value": 40
}
```
- Output Message:
```json
{
  "mtype": "debug", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Successfully changed baseline_vibration to 40"
}
```

### Input Message Tests

**T-INPUT-01**: Change to RUNNING state.
- Input Message:
```json
{
  "mtype": "vibration_sensor.start", 
  "from": "brain", 
  "to": "vibration_sensor1" 
}
```
- Output Message:
```json
{
  "mtype": "debug", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "vibration_sensor changed to RUNNING state"
}
```
**T-INPUT-02**: Change to STOP state.
- Input Message:
```json
{
  "mtype": "vibration_sensor.stop", 
  "from": "brain", 
  "to": "vibration_sensor1" 
}
```
- Output Message:
```json
{
  "mtype": "debug", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "vibration_sensor changed to STOP state"
}
```
**T-INPUT-03**: Change to ALERT state.
- Input Message:
```json
{
  "mtype": "vibration_sensor.alert", 
  "from": "brain", 
  "to": "vibration_sensor1" 
}
```
- Output Message:
```json
{
  "mtype": "debug", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "vibration_sensor changed to ALERT state"
}
```
## Error Handling Tests
**T-ERROR-01**: Missing "mtype" key.
- Input Message:
```json
{
  "from": "brain", 
  "to": "vibration_sensor1" 
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Missing 'mtype' key."
}
```
**T-ERROR-02**: Invalid type for "value" key.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "threshold", 
  "value": "high"
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "'value' must be an integer."
}
```
**T-ERROR-03**: Invalid parameter name.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "invalid_parameter", 
  "value": "10"
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Invalid name value"
}
```

**T-ERROR-04**: Threshold value out of range.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "threshold", 
  "value": "150"
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "'threshold' must be between 0 and 100"
}
```
**T-ERROR-05**: Missing "value" key from a config message.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": "threshold"
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Missing 'value' key"
}
```
**T-ERROR-06**: "name" value is not a string.
- Input Message:
```json
{
  "mtype": "config", 
  "from": "brain", 
  "to": "vibration_sensor1", 
  "name": 123,
  "value": 20
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "'name' must be a string"
}
```

**T-ERROR-07**: Unrecognized "mtype" value.
- Input Message:
```json
{
  "mtype": "vibration_sensor.pause", 
  "from": "brain", 
  "to": "vibration_sensor1"
}
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Invalid mtype value"
}
```
**T-ERROR-08**: Empty message or newline/carriage return only.
- Input Message:
```json
(empty or only \n\r)
```
- Output Message:
```json
{
  "mtype": "error", 
  "to": "*", 
  "from": "vibration_sensor1", 
  "message": "Deserialization failed: EmptyInput"
}
```








## User Story

GitHub Issue: [Vibration Sensor Integration](https://github.com/ComputerScienceUniversityofDenver/S25-EmbeddedSecuritySystem/issues/3)
