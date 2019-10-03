
#include <EEPROM.h>
#include <PJON.h>
#include <OSPREYSlave.h>

// Bus id definition
uint8_t bus_id[] = {0, 0, 0, 1};

// PJON object
OSPREYSlave<SoftwareBitBang> slave(bus_id);

/* A string is used to signal if the slave
   was previously initialized */
char initializer[] = "SLAVE";
uint32_t eeprom_rid = 0;

// State of the device id aquisition
bool acquired = false;

// Check if the device was previously initialized
bool is_initialized() {
  for(uint8_t i = 0; i < 5; i++)
    if(initializer[i] != EEPROM.read(i)) return false;
  return true;
}

void write_default_configuration() {
  for(uint8_t i = 0; i < 5; i++)
    EEPROM.update(i, initializer[i]);
  // Clean memory where to store RID
  EEPROM.update(5, 0);
  EEPROM.update(6, 0);
  EEPROM.update(7, 0);
  EEPROM.update(8, 0);
};

void receiver_handler(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  Serial.print("Received: ");
  for(uint16_t i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.flush();
};

void error_handler(uint8_t code, uint16_t data, void *custom_pointer) {
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("Connection lost with device ");
    Serial.println((uint8_t)slave.packets[data].content[0], DEC);
  }
  if(code == OSPREY_ID_ACQUISITION_FAIL) {
    if(data == OSPREY_ID_CONFIRM)
      Serial.println("OSPREYSlave error: master-slave id confirmation failed.");
    if(data == OSPREY_ID_NEGATE)
      Serial.println("OSPREYSlave error: master-slave id release failed.");
    if(data == OSPREY_ID_REQUEST)
      Serial.println("OSPREYSlave error: master-slave id request failed.");
  }
  Serial.flush();
};

void setup() {
  if(!is_initialized()) write_default_configuration();
  else slave.set_rid(EEPROM.get(5, eeprom_rid));
  Serial.begin(115200);
  slave.set_error(error_handler);
  slave.set_receiver(receiver_handler);
  slave.strategy.set_pin(12);
  slave.begin();
  slave.request_id();
}

void loop() {
  if((slave.device_id() != PJON_NOT_ASSIGNED) && !acquired) {
    Serial.print("Acquired device id: ");
    Serial.println(slave.device_id());
    Serial.flush();
    acquired = true;
    EEPROM.put(5, slave.get_rid());
  }
  slave.update();
  slave.receive(5000);
};
