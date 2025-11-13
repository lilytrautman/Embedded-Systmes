
T-CONFIG-01
Test runs properly with output data as expected.

- You can set the value to negative numbers and huge numbers. Maybe put in some guard rails for the values it will accept. It can also be 0 which goes against ur second configuration parameter blurb.

T-CONFIG-02
Test runs properly with output data as expected.

T-CONFIG-03
Test runs properly with output data as expected.

- Formatting on output message is weird in README on this one.

T-CONFIG-04
Test runs properly with output data as expected.

T-INPUT-01
Runs properly with output data as expected.

T-INPUT-02
Runs properly with output data as expected.

T-INPUT-03
Runs properly with output data as expected.

T-ERROR-01
{"from": "brain", "to": "vibration_sensor1"} gives the expected output error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"Missing 'mtype' key. "}

T-ERROR-02
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "threshold", "value": "high"} gives the expected output error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"'value' must be an integer. "}

T-ERROR-03
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "invalid_parameter", "value": "10"} does not give the expected output error and instead gives {"mtype":"error","to":"*","from":"vibration_sensor1","message":"'value' must be an integer. "}

- FIX:
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "invalid_parameter", "value": 10} gives the expected error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"Invalid 'name' value. "}

T-ERROR-04
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "threshold", "value": "150"} does not give the expected output error and instead gives {"mtype":"error","to":"*","from":"vibration_sensor1","message":"'value' must be an integer. "}

- FIX:
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": "threshold", "value": 150} gives the expected error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"'threshold' must be between 0 and 100. "}

T-ERROR-05
{"mtype": "config","from": "brain","to":"vibration_sensor1","name": "threshold"} gives expected output error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"Missing 'value' key. "}

- There is a period in the actual output and not on the readme but not really a big deal.

T-ERROR-06
{"mtype": "config", "from": "brain", "to": "vibration_sensor1", "name": 123,"value": 20} gives expected output error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"'name' must be a string. "}

T-ERROR-07
{"mtype": "vibration_sensor.pause", "from": "brain", "to": "vibration_sensor1"} gives expected output error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"Invalid mtype value. "}

T-ERROR-08
Entering nothing gives the output error of {"mtype":"error","to":"*","from":"vibration_sensor1","message":"Deserialization failedEmptyInput"} as expected.



Issues I found beyond the other quick fixes:

Maybe explain what each parameter is doing more, I'm still a little unclear on what values baseline_vibration, sample_rate, threshold, and smoothing should be. If I want to accurately set up the system, maybe putting some default values in the readme or hardcoded to start.

If the program has a json like this:
``` {"mtype": "config",    "from": "brain",    "to": "vibration_sensor1",    "name": "baseline_vibration",    "value": 10 } ```

I can still get that command to run the same with this command:

```{"mtype": "config",    "from": "brain",    "to": "*",    "name": "baseline_vibration",    "value": 10 } ```

or with this:

```{"mtype": "config",    "from": "",    "to": "*",    "name": "baseline_vibration",    "value": 10 } ```

The "to" field is checking for either 'vibration_sensor1' or '*' and will run commands with either in that field. This works for any command.

If you send json without {} on the ends it will do nothing. This includes faulty json messages.

The "from" field doesn't need to be filled out to run commands as seen above.