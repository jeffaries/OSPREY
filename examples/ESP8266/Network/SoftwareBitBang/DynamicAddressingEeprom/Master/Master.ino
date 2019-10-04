
/* This let the master receive a packet while trying to send another.
   Avoids network instability */
#define PJON_RECEIVE_WHILE_SENDING_BLOCKING true
#include <PJON.h>
#include <OSPREYMaster.h>

// Bus id definition
uint8_t bus_id[] = {0, 0, 0, 1};
// PJON object - The Master device id is fixed to PJON_MASTER_ID or 254
OSPREYMaster<SoftwareBitBang> master(bus_id);

uint32_t t_millis;

void setup() {
  Serial.begin(115200);
  /* Let addressing procedure packets to be received by the receiver function
     to ease debugging or analysis */
  master.debug = true;

  master.strategy.set_pin(5);
  master.set_receiver(receiver_function);
  master.set_error(error_handler);
  master.begin();
  /* Send a continuous greetings every second
     to showcase the receiver function functionality if debug is active */
  if(master.debug)
    master.send_repeatedly(PJON_BROADCAST, "Master says hi!", 15, 2500000);
  t_millis = millis();
};

void error_handler(uint8_t code, uint16_t data, void *custom_pointer) {
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("PJON error: connection lost with device id ");
    Serial.println((uint8_t)master.packets[data].content[0], DEC);
  }
  if(code == OSPREY_ID_ACQUISITION_FAIL) {
    Serial.print("PJONMaster error: connection lost with slave id ");
    Serial.println(data, DEC);
  }
  if(code == OSPREY_DEVICES_BUFFER_FULL) {
    Serial.print("PJONMaster error: master devices' buffer is full with a length of ");
    Serial.println(data);
  }
};

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  /* Make use of the payload before sending something, the buffer where payload points to is
     overwritten when a new message is dispatched */

  // If debug is active prints addressing packet infromation
  if(packet_info.port == OSPREY_DYNAMIC_ADDRESSING_PORT && packet_info.sender_id!=PJON_NOT_ASSIGNED) {
    uint32_t rid =
      (uint32_t)(payload[1]) << 24 |
      (uint32_t)(payload[2]) << 16 |
      (uint32_t)(payload[3]) <<  8 |
      (uint32_t)(payload[4]);
    Serial.print("Addressing request: ");
    Serial.print(payload[0]);
    Serial.print(" RID: ");
    Serial.print(rid);
  }

  // General packet data
  Serial.print(" Header: ");
  Serial.print(packet_info.header, BIN);
  Serial.print(" Length: ");
  Serial.print(length);
  Serial.print(" Sender id: ");
  Serial.print(packet_info.sender_id);

  // Packet content
  Serial.print(" Packet: ");
  for(uint8_t i = 0; i < length; i++) {
    Serial.print(payload[i]);
    Serial.print(" ");
  }
  Serial.print("Packets in buffer: ");
  Serial.print(master.update());
  Serial.print(" Devices in buffer: ");
  Serial.println(master.count_slaves());
};

void loop() {
  if(millis() - t_millis > 5000) {
    // Check if registered slaves are still present on the bus
    Serial.println("List of slaves known by master: ");
    for(uint8_t i = 0; i < OSPREY_MAX_SLAVES; i++) {
      if(master.ids[i].state) {
        /* ping slaves to make sure they are still connected */
        master.send(i+1,"Ping",1);
        Serial.print(" - Device id: ");
        Serial.print(i + 1); // Shifted by one to avoid PJON_BROADCAST
        Serial.print(" - Device RID: ");
        Serial.print(master.ids[i].rid);
        Serial.println();
      }
    }
    Serial.println();
    Serial.flush();
    t_millis = millis();
  }
  master.receive(5000);
  master.update();
};
