
            /*\      __   __   __   __   __
            shs-    |  | |__  |__| |__| |__  \ /
           dM_d:    |__|  __| |    |  \ |__   |  0.1
          dL:KM     Arduino compatible open-source bus network framework
         dM56Mh     based on the PJON standard. Giovanni Blu Mitolo 2016
        yM87MM:     gioscarab@gmail.com
        dgfi3h-
         NM*(Mm          /|  Copyright (c) 2014-2016,
     ___yM(U*MMo        /j|  Giovanni Blu Mitolo All rights reserved.
   _/OF/sMQWewrMNhfmmNNMN:|  Licensed under the Apache License, Version 2.0 (the "License");
  |\_\+sMM":{rMNddmmNNMN:_|  you may not use this file except in compliance with the License.
         yMMMMso         \|  You may obtain a copy of the License at
         gtMfgm              http://www.apache.org/licenses/LICENSE-2.0
        mMA@Mf
        MMp';M      Unless required by applicable law or agreed to in writing, software
        ysM1MM:     distributed under the License is distributed on an "AS IS" BASIS,
         sMM3Mh     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied
          dM6MN     See the License for the specific language governing permissions and
           dMtd:    limitations under the License.
            \*/

#include "OSPREY.h"

OSPREY::OSPREY() {
  for(int b = 0; b < MAX_BUSES; b++)
    for(int r = 0; r < MAX_KNOWN_DEVICES; r++) {
      buses[b]->known_devices[r].active = false;
      buses[b]->known_devices[r].id = NOT_ASSIGNED;
    }

  // TODO - Set receiver
  // TODO - Set error
};


/* Add a bus to the OSPREY bus list. The bus will be automatically handled by
   OSPREY: */

uint8_t OSPREY::add_bus(Link *link) {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(!buses[b]->active) {
      buses[b]->active = true;
      buses[b]->link = link;
      buses[b]->link->set_router(router);
      // TODO - Manage auto-addressing strategy
    }
};


/* Count the active buses in the OSPREY bus list: */

uint8_t OSPREY::count_active_buses() {
  uint8_t result = 0;
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b]->active)
      result++;
  return result;
};


/* Handle PJON packet state change: */

void OSPREY::handle_packet(uint8_t *bus_id, uint8_t packet_index, uint8_t state) {

};


/* Receive from all the added PJON buses for a predefined duration: */

void OSPREY::receive(uint32_t duration) {
  uint32_t time_per_bus = duration / count_active_buses();

  for(uint8_t b = 0; b < MAX_BUSES; b++) {
    uint32_t time = micros();
    if(buses[b]->active)
      while((uint32_t)(micros() - time) < time_per_bus)
        buses[b]->link->receive();
  }
};


void OSPREY::received(uint8_t *payload, uint8_t length, const PacketInfo &packet_info) {
  if(!(packet_info.header & ROUTING_BIT)) return; // return if non-OSPREY package
  uint8_t recipient_bus = find_bus(packet_info.receiver_bus_id, packet_info.receiver_id);

  if(recipient_bus != FAIL) {
    /* if(payload[11] == ACK) return remove_package_reference(packet_info.sender_bus_id, packet_info.sender_id);
    _receiver(payload, length, packet_info); Call OSPREY receiver
    return dispatch(DEFAULT_HEADER, recipient_bus, &packet_info.receiver_bus_id, packet_info.receiver_id, &packet_info.sender_bus_id, packet_info.sender_id, ACK, 0, package_id, ACK, 1); */
  }
};


uint16_t OSPREY::find_bus(const uint8_t *bus_id, uint8_t id) {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b]->active && bus_id_equality(bus_id, buses[b]->link->bus_id) && id == buses[b]->link->device_id())
      return b;
  return FAIL;
};


/*  PJON packet used to transmit an encapsulated OSPREY routing packet:

  |RX ID|      HEADER     |LENGTH|RX INFO| TX INFO |PACKET ID|HOPS|DATA|CRC|
  |_____|_________________|______|_______|_________|_________|____|____|___|
  |     |        |        |      |       |       | |         |    |    |   |
  | 1   |10001011|01000000|  18  |0.0.0.1|0.0.0.2|1|   99    | 1  | 64 |   |
  |_____|________|________|______|_______|_______|_|_________|____|____|___|
   ____________________________________________________________________________
  |                                                                            |
  |  Routed packet example:                                                    |
  |  Device 1 in bus 0.0.0.2 sends to device 1 in bus 0.0.0.1 the first        |
  |  packet since started (packet id 1) containing the content "@" or          |
  |  decimal 64.                                                               |
  |____________________________________________________________________________| */


uint16_t OSPREY::dispatch(
  uint8_t       bus_index,
  const uint8_t *recipient_bus_id,
  uint8_t       recipient_device_id,
  const uint8_t *sender_bus_id,
  uint8_t       sender_device_id,
  const char    *content,
  uint16_t      length,
  uint16_t      header = NOT_ASSIGNED,
  uint16_t      packet_id = 0,
  uint8_t       hops = 0
) {
  hops += 1;
  if(hops >= MAX_HOPS) return HOPS_LIMIT;

};


uint16_t OSPREY::send(
  const uint8_t *recipient_bus_id,
  uint8_t       recipient_device_id,
  const uint8_t *sender_bus_id,
  uint8_t       sender_device_id,
  const char    *content,
  uint16_t      length,
  uint16_t      header = NOT_ASSIGNED,
  uint16_t      packet_id = 0,
  uint8_t       hops = 0
) {
  int16_t package_id = generate_package_id();
  uint8_t header = DEFAULT_HEADER;

  // 1 First network id lookup with direct bus connections
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b]->active && bus_id_equality(buses[b]->link->bus_id, recipient_bus_id) || type == INFO) {
      // First level connection detected
      // Send OSPREY Package as PJON packet to the directly connected PJON bus
      /*dispatch(
        b,
        DEFAULT_HEADER | ACKNOWLEDGE_BIT,
        buses[b]->link->bus_id,
        buses[b]->link->device_id(),
        recipient_bus_id,
        recipient_device_id,
        type,
        package_id,
        content,
        length
      );*/
      if(type != INFO) return DISPATCHED;
    }

  if(type == INFO) return DISPATCHED;

  // 2 Network id lookup in every router's connected bus list
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b]->active)
      for(uint8_t d = 0; d < MAX_KNOWN_DEVICES; d++)
        if(buses[b]->known_devices[d].active)
          for(uint8_t k = 0; k < MAX_KNOWN_BUSES; k++)
            if(bus_id_equality(buses[b]->known_devices[d].known_bus_ids[k], recipient_bus_id)) {
              set_bit(header, 7); // Set ROUTE_REQUEST_BIT HIGH
              return DISPATCHED; //dispatch(
              /*  header,
                b,
                recipient_bus_id,
                recipient_device_id,
                sender_bus_id,
                sender_device_id,
                type,
                package_id,
                content,
                length
              );*/
              // Second level connection detected
              // Send OSPREY Package as PJON packet to the router connected to the target PJON bus
            }
  return BUS_UNREACHABLE;
};


void OSPREY::update() {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b]->active)
      buses[b]->link->update();
};
