
/*           /\      __   __   __   __   __
            shs-    |  | |__  |__| |__| |__  \ /
           dM_d:    |__|  __| |    |  \ |__   |  0.1
          dL:KM     Arduino compatible open-source mesh network protocol
         dM56Mh     based on the PJON standard. Giovanni Blu Mitolo 2015
        yM87MM:     gioscarab@gmail.com
        dgfi3h
        mMfdas-
         NM*(Mm          /|  Copyright (c) 2012-2015,
     ___yM(U*MMo        /j|  Giovanni Blu Mitolo All rights reserved.
   _/OF/sMQWewrMNhfmmNNMN:|  Redistribution and use in source and binary forms,
  |\_\+sMM":{rMNddmmNNMN:_|  with or without modification, are permitted provided
         yMMMMso         \|  that the following conditions are met:
         gtMfgm
        mMA@Mf    - Redistributions of source code must retain the above copyright
        MMp';M      notice, this list of conditions and the following disclaimer.
        ysM1MM:   - Redistributions in binary form must reproduce the above copyright
         sMM3Mh     notice, this list of conditions and the following disclaimer in the
          dM6MN     documentation and/or other materials provided with the distribution.
           dMtd:  - All advertising materials mentioning features or use of this software
            shs     must display the following acknowledgement:
             \/     "This product includes OSPREY software by Giovanni Blu Mitolo."

  - If a product includes / incorporates more than one product
    developed by the author, acknowledgements can be merged:
    "This product includes OSPREY and PJON software by Giovanni Blu Mitolo."

  - Neither the name of OSPREY, PJON, PJON_ASK nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

This software is provided by the copyright holders and contributors"as is"
and any express or implied warranties, including, but not limited to, the
implied warranties of merchantability and fitness for a particular purpose
are disclaimed. In no event shall the copyright holder or contributors be
liable for any direct, indirect, incidental, special, exemplary, or consequential
damages (in`cluding, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused and on any
theory of liability, whether in contract, strict liability, or tort (including
negligence or otherwise) arising in any way out of the use of this software, even if
advised of the possibility of such damage. */

#include "OSPREY.h"

// TODO - ALL HAS TO BE CHANGED, PACKAGES WILL BE SAVED ONLY IN THEIR STRING RAPRESENTATION
// A PACKAGE CLASS HAS TO BE IMPLEMENTED WITH GETTER METHODS TO REACH THE DESIRED PART OF THE PACKAGE CLEARLY

OSPREY::OSPREY() {
  for(int p = 0; p < MAX_PACKAGES; p++)
    packages[i].active = false;

  for(int b = 0; b < MAX_BUSES; b++) {
    for(int r = 0; r < MAX_KNOW_DEVICES; r++) {
      buses[b].known_devices[r].active = false;
    }
  }

  // TODO - Set receiver
  // TODO - Set error
};


uint8_t OSPREY::add_bus(Link l, uint8_t bus_id[BUS_ID_LENGTH], boolean router) {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(!buses[b].active) {
      buses[b].active = true;
      buses[b].link = l;
      buses[b].device.id = l.device_id();
      // TODO - Manage auto-addressing strategy

      for(uint8_t n = 0; n < BUS_ID_LENGTH; n++)
        buses[b].id[n] = bus_id[n];

      buses[b].device.router = router;
    }
};


uint8_t OSPREY::add_package(Package package) {
  for(uint8_t p = 0; p < MAX_PACKAGES; p++)
    if(!packages[p].active) {
      packages[p] = package;
      return TO_BE_DISPATCHED;
    }
    return PACKAGE_BUFFER_FULL;
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
   _________________________________________________________________________________
  | TYPE | RECIPIENT INFO       | SENDER INFO          | PACKAGE INFO     | CONTENT |
  |______|______________________|______________________|__________________|_________|
  |      | bus_id          | id | bus_id          | id | hops | packet id |         |
  |______|_________________|____|_________________|____|______|___________|_________|
  |  __  |  __  __  __  __ | __ |  __  __  __  __ | __ |  __  |  __  __   |   __    |
  | |  | | |  ||  ||  ||  |||  || |  ||  ||  ||  |||  || |  | | |  ||  |  |  |  |   |
  | |__| | |__||__||__||__|||__|| |__||__||__||__|||__|| |__| | |__||__|  |  |__|   |
  |  102 |  0 . 0 . 0 . 1  | 1  |  0 . 0 . 0 . 2  | 1  |   1  |   0   1   |   64    |
  |______|_________________|____|_________________|____|______|___________|_________|
  |  Package example:                                                               |
  |  Device 1 in bus 0.0.0.2 is sending a REQUEST (value 102)                       |
  |  to device 1 in bus 0.0.0.1 the first package since started                     |
  |  (package id 1) containing the content "@" or decimal 64                        |
  |_________________________________________________________________________________| */


uint8_t OSPREY::send(Package p) {

  // 1 First network id lookup with direct bus connections
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      if(bus_id_equality(buses[b].id, p.content[0])) {
        // First level connection detected
        // Send OSPREY Package as PJON packet to the directly connected PJON bus
        p.packet_id = buses[b].link.send(p.content[BUS_ID_LENGTH], p.content, p.length);
        p.state = TO_BE_DISPATCHED;
        return this->add_package(p);
      }

  // 2 Network id lookup in every router's connected bus list
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      for(uint8_t d = 0; d < MAX_KNOW_DEVICES; d++)
        if(buses[b].known_devices[d].active && buses[b].known_devices[d].router)
          for(uint8_t k = 0; k < MAX_KNOWN_BUSES; k++)
            if(bus_id_equality(buses[b].known_devices[d].known_bus_ids[k], p.content[0])) {
              // Second level connection detected
              // Send OSPREY Package as PJON packet to the router connected to the target PJON bus
              p.packet_id = buses[b].link.send(buses[b].known_devices[d].id, content, length);
              p.state = TO_BE_DISPATCHED;
              return this->add_package(p);
            }

  // 3 Network id lookup in all uknown networks
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      for(uint8_t d = 0; d < MAX_KNOW_DEVICES; d++)
        if(buses[b].known_devices[d].active && buses[b].known_devices[d].router) {
          // TODO - Manage somehow :)
          p.packet_id = buses[b].link.send(buses[b].known_devices[d].id, content, length);
          p.state = TO_BE_DISPATCHED;
          return this->add_package(p);
        }

  return DESTINATION_BUS_UNREACHABLE;
};


void OSPREY::update() {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      buses[b].link.update();
};


void OSPREY::receive(uin32_t duration) {
  uint8_t  buses = this->count_active_buses();
  uint32_t time_per_bus = duration / buses;

  for(uint8_t b = 0; b < MAX_BUSES; b++) {
    uint32_t time = micros();
    if(buses[b].active)
      while((uint32_t)(time + time_per_bus) >= micros())
        buses[b].link.receive();
  }
};


void OSPREY::received(uint8_t length, uint8_t *payload) {
  // Check if package's recipient is this device on one of its buses
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      if(bus_id_equality([payload, buses[b].id)) {
        if(payload[BUS_ID_LENGTH] == buses[b].device.id)
          this->_receiver(length - 14, payload[13]);
          // TODO - send acknowledgement back
        if(buses[b].device.router)
          for(uint8_t k = 0; k < MAX_KNOW_DEVICES; k++)
            if(buses[b].known_devices[k].id ==  payload[BUS_ID_LENGTH])
              // TODO - Send package through link
      }
};


uint8_t OSPREY::count_active_buses() {
  uint8_t buses;
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      buses++;
  return buses;
}


void free_package(uint8_t id) {
  packages[id].attemps = 0;
  packages[id].hops = 0;
  packages[id].state = NULL;
  packages[id].tx_id = 0;
};


uint8_t OSPREY::generate_package_id(uint8_t bus_id) {
  if (_package_id_source + 1 > _package_id_source)
    return _package_id_source++;
  return 0;
};


boolean OSPREY::bus_id_equality(uint8_t *id_one, uint8_t *id_two) {
  for(uint8_t i = 0; i < BUS_ID_LENGTH; i++)
    if(id_one[i] != id_two[i])
      return false;
  return true;
};
