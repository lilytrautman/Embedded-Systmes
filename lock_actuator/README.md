# Lock Actuator

## User Story
As a Vault Actuator I want to respond to a command so that I can lock the trophy enclosure (display case?) to restrict access to a vault

https://github.com/ComputerScienceUniversityofDenver/S25-EmbeddedSecuritySystem/issues/9

## Configuration Messages/Parameters:
"some_angle" will be a numeric value (likely an integer) and this may only need
configuration depending on hardware in which case it could be hardcoded
This parameter represents how far the motor must move to lock
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"some_angle" }
```
These were the original config parameters, they were redundant so I combined them into one
for the final implementation.
```json
{ "mtype":"config", "from":"does_not_matter", "to":"my_unique_id", 
  "name":"locked_position", "value":"some_angle" }
{ "mtype":"config", "from":"does_not_matter", "to":"my_unique_id", 
"name":"unlocked_position", "value":"some_angle" }
```

## Output/Debug/Error Messages:
```json
{ "mtype":"error", "from": "my_unique_id", "to":"*", "message":"failed to lock"}
{ "mtype":"lock_actuator1.state", "from": "my_unique_id", "to":"*", "message":"the current state is current_state"}
{ "mtype":"lock_actuator1.state_change", "from": "my_unique_id", "to":"*", "message":"state changed from prev_state to new_state"}
```

## Input Messages:
```json
{ "mtype":"lock_actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
{ "mtype":"lock_actuator1.unlock", "from":"sender_unique_id", "to":"lock_actuator1" }
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock_actuator1" }
```

## Hardare Connections:
### Hardware Setup Instructions
- Power will come from a standard wall outlet connection directly into the motor controller
- Connect GND from ESP32 to (-) rail on breadboard.
- Connect PUL+ wire (green wire) to pin D5 and connect DIR+ wire(yellow wire) to pin D4 on ESP32.
- Connect all remaining blue wires (ENA-, ENA+, DIR-, PUL-) to (-) rail on breadboard.
- The motor controller High Voltage channels (A+, A-, B+, B-) should already be connected to the motor through the black wire coming out of the motor.
- VCC and GND from the motor controller should be connected to the external power supply coming from a wall outlet.
### Hardware Updates and Challenges
- IMPORTANT UPDATE: pin D6 does not work for this purpose as it is used on startup and effects the motor when the program is being downloaded. I switched DIR+ to pin D4 and PUL+ to pin D5
- the test sketch simply has the motor move in one direction at a constant speed, there are const variables that can be changed before compilation that will change the speed and direction
- the main challenges I faced while coding this mostly revolved around controlling the timing of the motor moving. It was a bit challenging to implement the state toggle since there had to be a delay in between every state change, but it was not too difficult to figure out.

## Software Design:
- needs to be able to set the position which is considered locked according to configuration messages
- needs to respond to commands which lock or unlock by performing the associated action, and should send a message with information about the changed state when done
- needs to take config parameter to change degree necessary to be considered locked
- if locking or unlocking fails, an error message should be sent indicating the failure

## limitations of current implementation:
- cannot set a specific coordinate or radian to rotate motor towards, it only rotates from where it starts when activated

## Documentation Issues Encountered in Testing and QA
- (RESOLVED) Hardware Connections section should be cleaned up and made more user-friendly. Some of it includes outdated instructions.
- lock_actuator_hardware.png should be updated to reflect new hardware design.
- (RESOLVED)Test case descriptions' formatting can be improved, should include an oracle for commands that move the motor.


## Test Cases:
**T-CONFIG-01:** Tests normal behavior of changing degree_to_lock to 360.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"360" }
```
Output: lock correctly turns 360 degrees when told to lock after this command is sent.

**T-CONFIG-02:** Tests that lock position cannot be changed while locked.

Input: 
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"180" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Error, lock_actuator1 config name does not match accepted name or device is locked"}
```
**T-CONFIG-03:** Tests that changing the lock position by an invalid value gives an error.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"cat" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Invalid Degree entered"}
```

**T-CONFIG-04:** Confirms config messages with a "to" field that is not "door_actuator1" are ignored.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"not_the_lock_actuator", 
  "name":"degree_to_lock", "value":"cat" }
```
Output: nothing, message is ignored.

**T-INPUT-01:** Confirms that when system is unlocked and a lock command is given, the device will lock by rotating it by specified degree in config.

Input:
```json
  { "mtype":"lock_actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Oracle: Motor rotates specified degrees in config.

Output:
```json
{"mtype":"lock_actuator1.state_change","from":"lock_actuator1","to":"*","message":"State of lock changed, locked = 1"}
```
**T-INPUT-02:** Confirms that when in locked state and just received a lock command, motor does not move and an error message is broadcasted.

Input:
```json
{"mtype":"lock_actuator1.lock","from":"does_not_matter","to":"lock_actuator1"}
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"lock_actuator1 is already locked"}
```

**T-INPUT-03:** Confirms that when in locked state and receives an unlock command, motor should rotate some degrees to reach unlocked state.

Input:
```json
{ "mtype":"lock_actuator1.unlock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"lock_actuator1.state_change","from":"lock_actuator1","to":"*","message":"State of lock changed, locked = 0"}
```

**T-INPUT-04:** Confirms that unlock command does not do anything when already in unlocked state.

Input:
```json
{ "mtype":"lock_actuator1.unlock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"lock_actuator1 is already unlocked"}
```

**T-INPUT-05:** Confirms that the request state input message works and correctly outputs the state when unlocked.

```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock_actuator1" }
```
- output
```json
{"mtype":"lock_actuator1.state","from":"lock_actuator1","to":"*","message":"State of lock, locked = 0"}
```
- correctly outputs the state of the lock
**T-INPUT-06:** Confirms that the request state input works when module is locked.

Input:
```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"lock_actuator1.state","from":"lock_actuator1","to":"*","message":"State of lock, locked = 1"}
```

**T-INPUT-07**: Tests to make sure request state command is not executed if it is not sent to lock_actuator1.

Input:
```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock" }
```
Output: Nothing, message is ignored.

**T-ERROR-01**: Tests to make sure giving a non-viable mtype returns an error.

Input:
```json
{ "mtype":"say_hello", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"error, no mtype command matches given mtype"}
```
**T-ERROR-02**: Tests to make sure mtype is actually present in input.

Input:
```json
{ "mnothing":"lock-actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Error, mtype is missing in command"}
```

**T-ERROR-03**: Tests to make sure that a message with a missing "to" field is ignored

Input:
```json
{ "mnothing":"lock-actuator1.lock", "from":"sender_unique_id", "wow":"lock_actuator1" }
```
Output: Nothing, message is ignored.

**T-ERROR-04**: Tests to make sure name was added to the config parameter and outputs an error.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "nombre":"degree_to_lock", "value":"360" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"error, name or value of config parameter was not provided"}
```

**T-ERROR-05**: Confirms the name is a valid config parameter for the device.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"coolness", "value":"100000" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Error, lock_actuator1 config name does not match accepted name or device is locked"}
```
- correctly outputs an error message