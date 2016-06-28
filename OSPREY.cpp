
            /*/\      __   __   __   __   __
            shs-    |  | |__  |__| |__| |__  \ /
           dM_d:    |__|  __| |    |  \ |__   |  0.1
          dL:KM     Arduino compatible open-source mesh network framework
         dM56Mh     based on the PJON standard. Giovanni Blu Mitolo 2016
        yM87MM:     gioscarab@gmail.com
        dgfi3h
        mMfdas-
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
            shs
            \/*/

#include "OSPREY.h"

OSPREY::OSPREY() {
  set_routing_handler(dummy_handler);

  for(int b = 0; b < MAX_BUSES; b++)
    for(int r = 0; r < MAX_KNOWN_DEVICES; r++)
      buses[b].known_devices[r].active = false;

  for(uint8_t p = 0; p < MAX_PACKAGE_REFERENCES; p++)
    package_references[p].package_id = 0;

  // TODO - Set receiver
  // TODO - Set error
};


/* Add a bus to the OSPREY bus list. The bus will be automatically handled by
   OSPREY: */

uint8_t OSPREY::add_bus(Link link, uint8_t *bus_id, uint8_t device_id, boolean router) {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(!buses[b].active) {
      buses[b].active = true;
      buses[b].link = link;
      buses[b].link.set_packet_auto_deletion(false);
      buses[b].link.set_router(router);
      // TODO - Manage auto-addressing strategy
    }
};


/* Create a reference between the dispatched PJON packet on a certain bus to a OSPREY package id: */

void OSPREY::add_package_reference(uint8_t bus_id[4], uint16_t package_id, uint8_t packet_index) {
  for(uint8_t p = 0; p < MAX_PACKAGE_REFERENCES; p++)
    if(!package_references[p].package_id) {
      for(uint8_t b = 0; b < 4; b++)
        package_references[p].bus_id[b] = bus_id[b];

      package_references[p].package_id = package_id;
      package_references[p].packet_index = packet_index;
    }
  // TODO - detect memory leak error
};


/* Compute bus id equality: */

boolean OSPREY::bus_id_equality(uint8_t *id_one, uint8_t *id_two) {
  for(uint8_t i = 0; i < 4; i++)
    if(id_one[i] != id_two[i])
      return false;
  return true;
};


/* Count the active buses in the OSPREY bus list: */

uint8_t OSPREY::count_active_buses() {
  uint8_t result = 0;
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      result++;
  return result;
};


/* Generate a auto-increment package id: */

uint16_t OSPREY::generate_package_id() {
  if (_package_id_source + 1 > _package_id_source)
    return _package_id_source++;
  return 1;
};


/* Handle PJON packet state change: */

void OSPREY::handle_packet(uint8_t bus_id[4], uint8_t packet_index, uint8_t state) {
  if(state == DISPATCHED /* && bus(bus_id).packets[packet_index].content[9] == ACK */) // So if ack
    remove_package_reference(bus_id, packet_index);
  // TODO - Hanle sending failure
};


/* Receive from all the added PJON buses for a predefined duration: */

void OSPREY::receive(uint32_t duration) {
  uint32_t time_per_bus = duration / count_active_buses();

  for(uint8_t b = 0; b < MAX_BUSES; b++) {
    uint32_t time = micros();
    if(buses[b].active)
      while((uint32_t)(time + time_per_bus) >= micros())
        buses[b].link.receive();
  }
};


void OSPREY::received(uint8_t id, uint8_t *payload, uint8_t length) {
  uint16_t package_id = payload[11] << 8 | payload[12] & 0xFF;
  uint8_t  recipient_bus_id[] = {payload[0], payload[1], payload[2], payload[3]};
  uint8_t  sender_bus_id[] = {payload[4], payload[5], payload[6], payload[7]};
  uint8_t  type = payload[9];

  // Check if package's recipient is this device on one of its buses
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      if(bus_id_equality(payload, buses[b].link.bus_id)) {
        if(id == buses[b].link.device_id()) {
          if(type == ACK)
            remove_package_reference(recipient_bus_id, package_id);

          _routing_handler(
            recipient_bus_id,                       // Recipient bus_id
            id,                                     // Recipient device_id
            sender_bus_id,                          // Sender    bus_id
            payload[8],                             // Sender    device_id
            type,                                   // Package type
            payload[10],                            // Hops
            package_id,                             // Package id
            payload + 13,                           // Package content
            length - 13                             // Length - header
          );

          // return Send back acknowledgement
          char response[] = { ACK };

          send(
            *sender_bus_id,
            payload + 8,
            *recipient_bus_id,
            id,
            ACK,
            payload[11] << 8 | payload[12] & 0xFF,
            response,
            1
          );
        }
      }
};


void OSPREY::remove_package_reference(uint8_t bus_id[4], uint16_t package_id) {
  for(uint8_t p = 0; p < MAX_PACKAGE_REFERENCES; p++)
    if(bus_id_equality(package_references[p].bus_id, bus_id) && package_references[p].package_id == package_id) {
      package_references[p].bus_id[0] = 0;
      package_references[p].bus_id[1] = 0;
      package_references[p].bus_id[2] = 0;
      package_references[p].bus_id[3] = 0;
      package_references[p].packet_index = 0;
      package_references[p].package_id = 0;
      for(uint8_t b = 0; b < MAX_BUSES; b++)
        if(bus_id_equality(package_references[p].bus_id, buses[b].bus_id)
          buses[b].remove(package_references[p].packet_index);
    }
};


/*  PJON packet used to transmit an encapsulated OSPREY package
   ________________________________________
  |   _______ ________ _________ _______   |
  |  |       |        |         |       |  |
  |  |  id   | length | content |  CRC  |  |
  |  |_______|________|_________|_______|  |
  |________________________|_______________|
                           |
                           |
  PJON bus is used as a link for an OSPREY network.
  Here an example of OSPREY package:
   ____________________________________________________________________________
  | RECIPIENT INFO  | SENDER INFO          | PACKAGE INFO            | CONTENT |
  |_________________|______________________|_________________________|_________|
  | bus_id          | bus_id          | id | type | hops | packet id |         |
  |_________________|_________________|____|______|______|___________|_________|
  |  __  __  __  __ |  __  __  __  __ | __ |  __  |  __  |  __  __   |   __    |
  | |  ||  ||  ||  || |  ||  ||  ||  |||  || |  | | |  | | |  ||  |  |  |  |   |
  | |__||__||__||__|| |__||__||__||__|||__|| |__| | |__| | |__||__|  |  |__|   |
  |  0 . 0 . 0 . 1  |  0 . 0 . 0 . 2  | 1  |  102 |   1  |   0   1   |   64    |
  |_________________|_________________|____|______|______|___________|_________|
  |                                                                            |
  |  Package example:                                                          |
  |  Device 1 in bus 0.0.0.2 is sending a REQUEST (value 102) to device 1      |
  |  (present in the lower level PJON's packet) in bus 0.0.0.1 the first       |
  |  package since started (package id 1) containing the content "@" or        |
  |  decimal 64.                                                               |
  |____________________________________________________________________________| */


uint16_t OSPREY::send(
  Link     link,
  uint8_t  *bus_id,
  uint8_t  device_id,
  uint8_t  type,
  uint8_t  hops,
  uint16_t package_id,
  char     *content,
  uint8_t  length
) {
  uint16_t packet;
  char *payload = (char *) malloc(length + 12);

  if(payload == NULL) return FAIL;

  memcpy(payload, bus_id, 4);
  memcpy(payload + 4, link.bus_id, 4);
  payload[8] = link.device_id();
  payload[9] = type;
  payload[10] = (hops < MAX_HOPS) ? hops + 1 : 0;
  // TODO - detect max hops, send back HOPS_LIMIT error package
  payload[11] = package_id  >> 8;
  payload[12] = package_id & 0xFF;
  memcpy(payload + 13, content, length);
  add_package_reference(bus_id, package_id, link.send(device_id, payload, length + 12));
  free(payload);
  return packet;
};


uint16_t OSPREY::send(uint8_t *bus_id, uint8_t device_id, uint8_t type, char *content, uint8_t length, uint8_t hops) {
  int package_id = generate_package_id();
  // 1 First network id lookup with direct bus connections
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      if(bus_id_equality(buses[b].link.bus_id, bus_id)) {
        add_package_reference(bus_id, package_id, send(b, bus_id, device_id, type, hops, package_id, content, length));
        return DISPATCHED;
        // First level connection detected
        // Send OSPREY Package as PJON packet to the directly connected PJON bus
      }
  // 2 Network id lookup in every router's connected bus list
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      for(uint8_t d = 0; d < MAX_KNOWN_DEVICES; d++)
        if(buses[b].known_devices[d].active)
          for(uint8_t k = 0; k < MAX_KNOWN_BUSES; k++)
            if(bus_id_equality(buses[b].known_devices[d].known_bus_ids[k], bus_id)) {
              add_package_reference(bus_id, package_id, send(b, bus_id, device_id, type, hops, package_id, content, length));
              return DISPATCHED;
              // Second level connection detected
              // Send OSPREY Package as PJON packet to the router connected to the target PJON bus
            }
  return BUS_UNREACHABLE;
};


void OSPREY::set_routing_handler(routing_handler h) {
  _routing_handler = h;
};


void OSPREY::update() {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      buses[b].link.update();
};
