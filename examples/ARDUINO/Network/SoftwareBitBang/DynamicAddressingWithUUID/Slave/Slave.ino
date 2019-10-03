#include <PJON.h>
#include <OSPREYSlave.h>

// Bus id definition
uint8_t bus_id[] = {0, 0, 0, 1};

// PJON object
OSPREYSlave<SoftwareBitBang> slave(bus_id);

int packet;
char content[] = "01234567890123456789";
bool acquired = false;

void receiver_handler(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  Serial.print("Received: ");
  for(uint16_t i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  } 
  Serial.print(" -> ");
  Serial.print(slave.device_id());
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
  Serial.begin(115200);
   slave.set_rid(87654321);
  slave.set_error(error_handler);
  slave.set_receiver(receiver_handler);
  slave.strategy.set_pin(5);
  slave.begin();
  slave.request_id();
}

void loop() {
  if((slave.device_id() != PJON_NOT_ASSIGNED) && !acquired) {
    Serial.print("Acquired device id: ");
    Serial.println(slave.device_id());
    Serial.flush();
    acquired = true;
  }
  slave.update();
  slave.receive(5000);
};
