

## Documentation
`OSPREYMaster` and `OSPREYSlave` classes implement the master-slave [dynamic addressing](/specification/OSPREY-dynamic-addressing-specification-v0.1.md) features on top of the `PJON` class. Slaves can be connected to a bus, they can be hot-swapped, their id is automatically assigned by the master and their presence can be optionally monitored. The Master keeps an updated list of known slaves.    

### OSPREYMaster
The `OSPREYMaster` class implement master's dynamic addressing procedure which is totally automatic a requires no configuration, although some optional configuration is available. The constant `OSPREY_MAX_SLAVES` can be used to predefine the maximum amount of slaves handled by the master:  
```cpp
#define OSPREY_MAX_SLAVES 50
#include <OSPREYMaster.h>          // Include OSPREYMaster class
```
The `OSPREYMaster` class can be used in both local and shared mode:
```cpp
// Local mode instantiation
OSPREYMaster<SoftwareBitBang> bus;
// Shared mode instantiation
uint8_t bus_id[] = {0, 0, 0, 1};
OSPREYMaster<SoftwareBitBang> bus(bus_id);
```
If debugging info is needed set the state of `bus.debug` accordingly:
```cpp
bus.debug = true;
```

This is the list of the addressing errors possibly returned by the error call-back:

- `OSPREY_ID_ACQUISITION_FAIL` (value 105), `data` parameter contains lost packet's id.
- `OSPREY_DEVICES_BUFFER_FULL` (value 254), `data` parameter contains slaves buffer length.

This is an example of how an error call-back can be defined:
```cpp

void setup() {
  bus.set_error(error_function);
}

void error_function(uint8_t code, uint8_t data, void *custom_pointer) {
  // Standard PJON error
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("Connection lost with device ");
    Serial.println((uint8_t)bus.packets[data].content[0], DEC);
  }
  // OSPREYMaster related errors
  if(code == OSPREY_ID_ACQUISITION_FAIL) {
    Serial.print("Connection lost with device ");
    Serial.println(data, DEC);
  }
  if(code == OSPREY_DEVICES_BUFFER_FULL) {
    Serial.print("Master devices buffer is full with a length of ");
    Serial.println(data);
  }
};
```

### OSPREYSlave
Use the `OSPREYSlave` class for slaves in both local and shared mode:
```cpp
#include <OSPREYSlave.h>
// Local mode instantiation
OSPREYSlave<SoftwareBitBang> bus;
// Shared mode instantiation
uint8_t bus_id[] = {0, 0, 0, 1};
OSPREYSlave<SoftwareBitBang> bus(bus_id, PJON_NOT_ASSIGNED);
```

Call `request_id()` to acquire an id assigned by master:
```cpp
bus.request_id();
```
This is the list of the addressing errors possibly returned by the error call-back:
- `OSPREY_ID_ACQUISITION_FAIL` (value 105), `data` parameter contains the failed request

Requests list:
- `OSPREY_ID_REQUEST` (value 200), master-slave id request
- `OSPREY_ID_CONFIRM` (value 201), master-slave id confirmation
- `OSPREY_ID_NEGATE`  (value 203), master-slave id release

This is an example of how an error call-back can be defined:
```cpp
void setup() {
  bus.set_error(error_function);
}

void error_handler(uint8_t code, uint8_t data, void *custom_pointer) {
  // Standard PJON error
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("Connection lost with device ");
    Serial.println((uint8_t)bus.packets[data].content[0], DEC);
  }
  // OSPREYSlave related errors
  if(code == OSPREY_ID_ACQUISITION_FAIL) {
    if(data == OSPREY_ID_CONFIRM)
      Serial.println("Master-slave id confirmation procedure failed.");
    if(data == OSPREY_ID_NEGATE)
      Serial.println("Master-slave id release procedure failed.");
    if(data == OSPREY_ID_REQUEST)
      Serial.println("Master-slave id request procedure failed.");
  }
};
```

See the [DynamicAddressing](../examples/ARDUINO/Network/SoftwareBitBang/DynamicAddressing) example for a working showcase.
