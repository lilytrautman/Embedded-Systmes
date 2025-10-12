# Testing log

## Config tests

### Exsisting tests

**T-CONFIG-01**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": 2000
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'calibration_time_ms' value: '2000' ran successfully."
}
```

**T-CONFIG-02**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "sample_rate_ms",
  "value": 200
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'sample_rate_ms' value: '200' ran successfully."
}
```

**T-CONFIG-03**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_threshold",
  "value": 20.0
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'manual_threshold' value: '20' ran successfully."
}
```

**T-CONFIG-04**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "mode",
  "value": "manual"
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'mode' value: 'manual' ran successfully."
}
```

**T-CONFIG-05**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_angle",
  "value": 45
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'manual_angle' value: '45' ran successfully."
}
```

**T-CONFIG-06**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "frequency_rate_ms",
  "value": 2000
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'frequency_rate_ms' value: '2000' ran successfully."
}
```

**T-CONFIG-07**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "smoothing_amount",
  "value": 3.0
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'smoothing_amount' value: '3' ran successfully."
}
```

## Input tests

**T-INPUT-01**

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.scan",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "start"
}
```

**Output JSON**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'scanning_ir_camera.scan' name: 'start' value: 'null' ran successfully."
}
```

If any change is detected that is above the threshold then a output is generated in the follwoing format.

```json
{
  "mtype": "scanning_ir_camera.motion_detected",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "motion_detected",
  "value": 65
}
```

---

**T-INPUT-02**

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.scan",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "stop"
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'scanning_ir_camera.scan' name: 'stop' value: 'null' ran successfully."
}
```

---

**T-INPUT-03**

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.get",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "heatmap"
}
```

**Output JSON:**

```json
{
  "mtype": "scanning_ir_camera.out",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "heatmap",
  "value": [
    22, 21.75, 21.5, 21.25, 21.75, 21.5, 21.75, 21.75, 21.75, 21, 21, 21, 21,
    21.5, 20.75, 21, 20, 20.25, 20.75, 20.75, 20.25, 20.75, 21, 21, 20, 20,
    20.25, 20, 20.25, 20.25, 20.25, 21.25, 20.25, 20, 19.75, 20.25, 19.5, 20,
    20.5, 20.75, 20.25, 19.75, 19.75, 19.5, 19.75, 20, 20, 21, 19.5, 19.5, 19.5,
    19, 19.75, 20, 20.75, 22, 19.5, 19.25, 19.25, 19.25, 19.25, 19.75, 18.75, 19
  ]
}
```

(values will be different)

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'scanning_ir_camera.get' name: 'heatmap' value: 'null' ran successfully."
}
```

---

**T-INPUT-04**

This test will verify that the module will send the average IR value when an input command with mtype `scanning_ir_camera.get` and name `average_ir` is issued.

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.get",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "average_ir"
}
```

**Output JSON:**

```json
{
  "mtype": "scanning_ir_camera.out",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "average_ir",
  "value": 20.50391
}
```

**Debug JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'scanning_ir_camera.get' name: 'average_ir' value: 'null' ran successfully."
}
```

---

**T-INPUT-05**

This test will verify that the module will send the current angle of the servo when an input command with mtype `scanning_ir_camera.get` and name `angle` is issued.

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.get",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "angle"
}
```

**Output JSON:**

```json
{
  "mtype": "scanning_ir_camera.out",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "angle",
  "value": 43
}
```

**Debug JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'scanning_ir_camera.get' name: 'angle' value: 'null' ran successfully."
}
```

Here is the **Error Handling Tests** section fully formatted according to your preferred style:

---

## Error Handling Tests

---

**T-ERROR-03**

**Input JSON:**

```json
{"not a real json: ? "}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Deserialization failed: InvalidInput"
}
```

---

**T-ERROR-04**

**Input JSON:**

```json
{
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": 2000
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "JSON does not contain 'mtype' key."
}
```

---

**T-ERROR-05**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "name": "calibration_time_ms",
  "value": 2000
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "JSON does not contain 'to' key."
}
```

---

**T-ERROR-06**

**Input JSON:**

```json
{
  "mtype": "config",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": 2000
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "JSON does not contain 'from' key"
}
```

---

**T-ERROR-07**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "value": 2000
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "JSON does not contain 'name' key."
}
```

---

**T-ERROR-08**

**Input JSON:**

```json
{
  "mtype": "NOT_SUPPORTED",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": 2000
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'mtype' value: NOT_SUPPORTED"
}
```

---

**T-ERROR-09**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms"
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "JSON does not contain 'value' key."
}
```

---

**T-ERROR-10**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "NOT_VALID",
  "value": 2000
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'name' value: NOT_VALID"
}
```

---

**T-ERROR-11**

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.scan",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "NOT_VALID"
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'name' value: NOT_VALID"
}
```

---

**T-ERROR-12**

**Input JSON:**

```json
{
  "mtype": "scanning_ir_camera.get",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "NOT_VALID"
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'name' value: NOT_VALID"
}
```

---

**T-ERROR-13**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": 0
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'calibration_time_ms' value: 0"
}
```

---

**T-ERROR-14**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "sample_rate_ms",
  "value": 50
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'sample_rate_ms' value: 50"
}
```

---

**T-ERROR-15**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_threshold",
  "value": 0.0
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'manual_threshold' value: 0.00"
}
```

---

**T-ERROR-16**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_angle",
  "value": 360
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'manual_angle' value: 360"
}
```

---

**T-ERROR-17**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "frequency_rate_ms",
  "value": 250
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'frequency_rate_ms' value: 250"
}
```

---

**T-ERROR-18**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "smoothing_amount",
  "value": 3000.0
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'smoothing_amount' value: 3000.00"
}
```

---

**T-ERROR-19**

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "mode",
  "value": "NOT A MODE"
}
```

**Error JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'mode' value: NOT A MODE"
}
```

## Additional Tests

---

**T-ADDITIONAL-01**
Verifies that the mode can be set to `"auto"`, enabling autonomous behavior.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "mode",
  "value": "auto"
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'mode' value: 'auto' ran successfully."
}
```

---

**T-ADDITIONAL-02**
Validates how the system handles a debug message simulating a config update.

**Input JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'calibration_time_ms' value: '2000' ran successfully."
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'mtype' value: debug"
}
```

---

**T-ADDITIONAL-03**
Tests if the system accepts an integer for a parameter typically expressed as a float.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_threshold",
  "value": 20
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'manual_threshold' value: '20' ran successfully."
}
```

---

**T-ADDITIONAL-04**
Tests rejection of extremely small `calibration_time_ms` values.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": 0.01
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'calibration_time_ms' value: 0"
}
```

---

**T-ADDITIONAL-05**
Ensures system rejects unreasonably large `sample_rate_ms` values.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "sample_rate_ms",
  "value": 100000
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'sample_rate_ms' value: 100000"
}
```

---

**T-ADDITIONAL-06**
Tests whether the system allows string values for numeric parameters.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "calibration_time_ms",
  "value": "2000"
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'calibration_time_ms' value: '2000' ran successfully."
}
```

---

**T-ADDITIONAL-07**
Checks if boolean values are accepted in place of expected floats.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_threshold",
  "value": true
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'manual_threshold' value: 'true' ran successfully."
}
```

---

**T-ADDITIONAL-08**
Tests acceptance of arrays where scalar numeric types are expected.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_angle",
  "value": [90]
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'manual_angle' value: '[90]' ran successfully."
}
```

---

**T-ADDITIONAL-09**
Verifies system behavior on null `value` fields.

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "frequency_rate_ms",
  "value": null
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'frequency_rate_ms' value: 0"
}
```

---

**T-ADDITIONAL-10**
Ensures `mtype` cannot be empty.

**Input JSON:**

```json
{
  "mtype": "",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "mode",
  "value": "auto"
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'mtype' value: "
}
```

---

**T-ADDITIONAL-11**
Validates proper config of `mode` to `auto` again (consistency check).

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "mode",
  "value": "auto"
}
```

**Output JSON:**

```json
{
  "mtype": "debug",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "successful_command",
  "message": "mtype: 'config' name: 'mode' value: 'auto' ran successfully."
}
```

---

**T-ADDITIONAL-12**
Checks if `mtype` casing is enforced (e.g., `Config` instead of `config`).

**Input JSON:**

```json
{
  "mtype": "Config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_angle",
  "value": 90
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'mtype' value: Config"
}
```

---

**T-ADDITIONAL-13**
Simulates input with potentially dangerous characters (e.g., script injection).

**Input JSON:**

```json
{
  "mtype": "config",
  "from": "*",
  "to": "scanning_ir_camera1",
  "name": "manual_threshold<script>",
  "value": 20.0
}
```

**Output JSON:**

```json
{
  "mtype": "error",
  "from": "scanning_ir_camera1",
  "to": "*",
  "name": "JSON_error",
  "message": "Unsupported 'name' value: manual_threshold<script>"
}
```

## Testing Summary

3. Documentation Quality Assurance

   - There were no syntax error in the test caase description.
   - Formating erros where not really errors but rather changine single line json blocks to multiline json blocks for better markdown formatting.
   - There were not ambigoius test cases as most of the test cases were either setting configs, sending commands, or sending invalid message to test error handling.
   - They are now properly formatterd.

4. Test case enhancement

- The only undefined or weird behaviour was related to the arduino `.as<Type>()` and how it would case booleans or arrays with single values to floats and ints.
- The additonal test were added and can be found int the [Additonal Tests](#additional-tests) section.
- There is a helpful python script that can run all the tests in `config.json` and `additional_test.json`. I didn't have time to create a automatic tested for input commands and error test, but since they are unnecessary it is not a big issue.
- Instructions on how to run the tester can be found in the [how to run tester](#how-to-run-testerpy) section.

## How to Run tester.py

To execute the `tester.py` script and run configuration tests on the scanning IR camera module, follow these steps:

1. upload your `ino` to the chip.
2. change the port on your arudino ide to any port that isn't the one you used to upload.
3. install pyserial `pip install pyserial`.
4. run the program as like so `python .\tester.py --port "COM6" ` use the same port you used to upload the `ino` file.
