## Configuration Parameter Tests
**T-CONFIG-01:** Tests normal behavior of setting degree_to_lock.
Input:
```json
{   "mtype":"config", 
    "from":"does_not_matter", 
    "to":"lock_actuator1", 
    "name":"degree_to_lock", 
    "value":"360" 
}
```
Expected output: Lock correctly turns 360 degrees when told to lock and debug message:
```json
{   "mtype":"debug", 
    "from":"lock_actuator_1", 
    "to":"*", 
    "message":"degree_to_lock set to 360"
}
```

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
Note: this doesn't output anything to serial when unlocked.

**T-CONFIG-03:** Tests that changing the lock position by an invalid value gives an error
Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"cat" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Invalid Degree entered"}
```

**T-CONFIG-04:** Tests that config messages to other modules are ignored.
Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"not_the_lock_actuator", 
  "name":"degree_to_lock", "value":"cat" }
```
Output: nothing, message should be ignored

**T-INPUT-01:** Tests that lock actuator correctly locks.
Input:
```json
  { "mtype":"lock_actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Oracle: Moves to locked state

Output:
```json
{"mtype":"lock_actuator1.state_change","from":"lock_actuator1","to":"*","message":"State of lock changed, locked = 1"}
```

**T-INPUT-02:** Tests to make sure that locking it while already locked produces error and does not move motor.

Input:
```json
  { "mtype":"lock_actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Oracle: Motor doesn't move.

Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"lock_actuator1 is already locked"}
```
**T-INPUT-03:** tests that the module correctly unlocks. 

Input: (input both commands if already in unlocked state)
```json
{ "mtype":"lock_actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
```json
{ "mtype":"lock_actuator1.unlock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Oracle: Motor moves to unlocked position

Output Message:
```json
{"mtype":"lock_actuator1.state_change","from":"lock_actuator1","to":"*","message":"State of lock changed, locked = 0"}
```
**T-INPUT-04:** Tests to make sure unlock command does not work if already unlocked.

Input: (input both commands if already in locked state)
```json
{ "mtype":"lock_actuator1.unlock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
```json
{ "mtype":"lock_actuator1.unlock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"lock_actuator1 is already unlocked"}
```

**T-INPUT-05:** Tests to see if the request state input works when unlocked.

Condition: Must be in unlocked state.

Input:
```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"lock_actuator1.state","from":"lock_actuator1","to":"*","message":"State of lock, locked = 0"}
```

**T-INPUT-06:** Tests to see if the request state input works when locked.

Condition: Must be in locked state.

Input:
```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"lock_actuator1.state","from":"lock_actuator1","to":"*","message":"State of lock, locked = 1"}
```

**T-INPUT-07:** Tests to make sure request state command is not executed if it is not sent to correct UID.

Input:
```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock" }
```
Output: Nothing

**T-ERROR-01:** Tests to make sure giving a non-viable mtype returns an error

Input:
```json
{ "mtype":"say_hello", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"error, no mtype command matches given mtype"}
```

**T-ERROR-02:** Tests to make sure mtype is actually present in input.

Input:
```json
{ "mnothing":"lock-actuator1.lock", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Error, mtype is missing in command"}
```

**T-ERROR-03:** Tests to make sure that a message with missing "to" field is ignored.

Input:
```json
{ "mnothing":"lock-actuator1.lock", "from":"sender_unique_id", "wow":"lock_actuator1" }
```
Output: nothing, message should be ignored.

**T-ERROR-04:** Tests to make sure name was added to the config message.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "nombre":"degree_to_lock", "value":"360" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"error, name or value of config parameter was not provided"}
```

**T-ERROR-05:** Tests to make sure the name is a valid config parameter for the device.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"coolness", "value":"100000" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Error, lock_actuator1 config name does not match accepted name or device is locked"}
```

**T-ADDITIONAL-01:** Tests that when unlocked and sent an invalid angle, it should output an out-of-range error.

Input:
```json
{ "mtype":"config", "from":"does_not_matter", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"-1" }
```
Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"Invalid Degree entered"}
```

**T-ADDITIONAL-02:** Tests to make sure a missing "from" field is ignored.

Input:
```json
{ "mtype":"config", "to":"lock_actuator1", 
  "name":"degree_to_lock", "value":"180" }
```
Output: Nothing, degree_to_lock parameter should change to 180.

**T-ADDITIONAL-03:** Tests that providing an invalid name for the "value" field is caught.

Input:
```json
{ "mtype":"config", "to":"lock_actuator1", "from":"does_not_matter",  "name":"degree_to_lock", "value_not":"180" }
```

Output:
```json
{"mtype":"error","from":"lock_actuator1","to":"*","message":"error, name or value of config parameter was not provided"}
```

**T-ADDITIONAL-04:** Confirms that when a request state input message is received, the module correctly outputs the state when locked.

Input:
```json
{ "mtype":"lock_actuator1.request_state", "from":"sender_unique_id", "to":"lock_actuator1" }
```
Output:
```json
{"mtype":"lock_actuator1.state_change","from":"lock_actuator1","to":"*","message":"State of lock changed, locked = 1"}
```











