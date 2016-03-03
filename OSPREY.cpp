
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
    _packages[i].active = false;

  for(int b = 0; b < MAX_BUSES; b++) {
    _buses[b].device.active = false;
    for(int r = 0; r < MAX_DEVICES; r++) {
      _buses[b].known_devices[r].active = false;
    }
  }

  // TODO - Set receiver
  // TODO - Set error
};


void OSPREY::add_bus(Link l, uint8_t bus_id[4], boolean router) {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(!_buses[b].active) {
      _buses[b].link = l;
      _buses[b].device.id = l.get_id();

      for(uint8_t n = 0; n < 4; n++)
        _buses[b].id[n] = bus_id[n];

      _buses[b].device.router = router;
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
  |_________________________________________________________________________________|*/


/*char * OSPREY::build_content(Package p, uint8_t id) {

  for(uint8_t i = 0; i < 4; i++)
    memory[i] = p.receiver_bus_id[i];
  memory[4] = p.receiver_device_id;

  for(uint8_t i = 0; i < 4; i++)
    memory[i + 5] = p.sender_bus_id[i];
  memory[9] = p.sender_device_id;

  if(id == PRE_EXISTENT)
    memory[10] = generate_package_id();
  else
    memory[10] = id;

  memory[11] = p.hops++;

  for(uint8_t c = 0; c < p.length; c++)
    memory[c + 12] = p.content;

  return memory;
};


Package OSPREY::create_package(uint8_t *content) {
  Package p;

  for(uint8_t i = 0; i < 4; i++)
    p.receiver_network_id[i] = content[i];

  p.receiver_device_id = content[4];

  for(uint8_t i = 0; i < 4; i++)
    p.sender_network_id[i] = content[i + 5];

  p.sender_device_id = content[9];

  p.id = content[10];
  p.hops = content[11]++; // TODO - Maybe ++content necessary

  for(uint8_t c = 0; c < p.length; c++)
    p.content[i] = content[c + 12];

  return p;
};*/


Ping OSPREY::ping(uint8_t bus_id[4], uint8_t device_id) {

};


Info OSPREY::info(Bus b) {

};


void OSPREY::receive(unsigned long duration) {
  duration = duration / _active_buses;
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(buses[b].active)
      buses[b].link.receive(duration);
};


void OSPREY::receive(uint8_t bus_index, unsigned long duration) {
  buses[bus_index].link.receive(duration);
};


void OSPREY::received(uint8_t length, uint8_t *payload) {
  // Check if package's recipient is this device on one of its buses
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(_buses[b].active)
      if(bus_id_equality([payload[0], payload[1], payload[2], payload[3]], _buses[b].id)) {
        if(payload[4] == _buses[b].device.id)
          this->_receiver(length - 14, payload[13]);
        // TODO - Route packages directed to known buses or devices
        // Manage somehow :)
      }
};


void OSPREY::assert(uint8_t bus_id[4], uint8_t device_id) {

}


void OSPREY::request(uint8_t bus_id[4], uint8_t device_id) {

}


void OSPREY::send(uint8_t bus_id[4], uint8_t device_id, uint8_t id = 0, uint8_t hops = 0) {
  // TODO - Check all offsets
  for(int p = 0; p < MAX_PACKAGES; p++)
    if(!_packages[p].active) {
      for(uint8_t i = 0; i < 4; i++)
        _packages[p].receiver_network_id[i] = bus_id[i];
      _packages[p].receiver_device_id = device_id[4];

      for(uint8_t i = 0; i < 4; i++)
        _packages[p].sender_network_id[i] = ?;
      _packages[p].sender_device_id = ?;
      // TODO - Which device id and network I shoudl choose in buses?

      _packages[p].id = id;
      _packages[p].hops = hops++;
      // TODO - Maybe ++hops necessary

      for(uint8_t c = 0; c < p.length; c++)
        p.content[c] = content[c + 12];

      _packages[p].state = TO_BE_DISPATCHED;
      return TO_BE_DISPATCHED;
    }

  return PACKAGE_BUFFER_FULL;
};


void OSPREY::send_package(Package p) {
  // 1 First network id lookup with direct bus connections
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(_buses[b].active)
      if(bus_id_equality(_buses[b].id, p.receiver_bus_id)) {
        // First level connection detected
        // Send OSPREY Package as PJON packet to the directly connected PJON bus
        p.packet_id = send_packet(_buses[b], p);
        return TO_BE_DISPATCHED;
      }

  // 2 Network id lookup in every router's connected bus list
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(_buses[b].active)
      for(uint8_t d = 0; d < MAX_DEVICES; d++)
        if(_buses[b].known_devices[d].active)
          for(uint8_t k = 0; k < MAX_KNOWN_BUSES; k++)
            if(bus_id_equality(_buses[b].known_devices[d].known_bus_ids[k], p.receiver_bus_id)) {
              // Second level connection detected
              // Send OSPREY Package as PJON packet to the router connected to the target PJON bus
              p.packet_id = send_packet(_buses[b], p, _buses[b].known_devices[d].id);
              return TO_BE_DISPATCHED;
            }

  // 3 Network id lookup in all uknown networks
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(_buses[b].active)
      for(uint8_t d = 0; d < MAX_DEVICES; d++)
        if(_buses[b].known_devices[d].active && _buses[b].known_devices[d].router) {
          p.packet_id = send_packet(_buses[b], p, _buses[b].known_devices[r].id);
          // TODO - Manage somehow :)
          return TO_BE_DISPATCHED;
        }

  return DESTINATION_BUS_UNREACHABLE;
};


int OSPREY::send_packet(uint8_t bus_id, Package p, uint8_t device_id = INHERIT) {
  // Send OSPREY Package as PJON packet in the connected PJON bus
  return buses[bus_id].link.send(
    (device_id == INHERIT) ? p.receiver_device_id : device_id,
    build_content(p),
    p.length
  );
};


void OSPREY::update() {
  for(uint8_t p = 0; p < MAX_PACKAGES; p++) {
    if(_packages[p].state == TO_BE_DISPATCHED) {
      send(_packages[p]);
    }
  }
};


void free_package(uint8_t id) {
  for(uint8_t i = 0; i < MAX_PACKAGE_LENGTH; i++)
    packages[id].content[i] = NULL;

  packages[id].attemps = 0;
  packages[id].hops = 0;
  packages[id].state = FREE;
};


uint8_t OSPREY::generate_package_id(uint8_t bus_id) {
  if (_package_id_source + 1 > _package_id_source)
    return _package_id_source++;
  else {
    _package_id_source = 0;
    return _package_id_source;
  }
};


boolean OSPREY::bus_id_equality(uint8_t id_one[4], uint8_t id_two[4]) {
  for(uint8_t i = 0; i < 4; i++)
    if(id_one[i] != id_two[i])
      return false;
  return true;
};
