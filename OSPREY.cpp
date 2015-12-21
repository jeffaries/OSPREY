
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
damages (including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused and on any
theory of liability, whether in contract, strict liability, or tort (including
negligence or otherwise) arising in any way out of the use of this software, even if
advised of the possibility of such damage. */

#include "OSPREY.h"


OSPREY::OSPREY() {
  for(int i = 0; i < MAX_PACKAGES; i++)
    _packages[i].active = false;

  for(int i = 0; i < MAX_BUSES; i++)
    _buses[i].active = false;

  // TODO - Set default known network_ids (0.0.0.0)
  // TODO - Set receiver
  // TODO - Set error
};


void OSPREY::add_bus(Transport t, uint8_t d, uint8_t n, boolean a = true) {
  for(uint8_t i = 0; i < MAX_BUSES; i++)
    if(_buses[i].state == FREE) {
      _buses[i].active = a;
      _buses[i].transport = t;
      _buses[i].device_id = d;
      for(uint8_t n_index = 0; n_index < 4; n_index++)
        _buses[i].network_id[n_index] = n[n_index];
    }
};



void OSPREY::update() {
  for(uint8_t p = 0; p < MAX_PACKAGES; p++)
    if(!_packages[p].state)
      send(_packages[p]);

  for(uint8_t b = 0; b < MAX_BUSES; p++)
    _buses[b].transport.receive((1000000 / (MAX_BUSES * 2)));
    // Dedicate 1.000.000 / (MAX_BUSES * 2) microseconds / second
    // to every bus reception
};


void OSPREY::send(package p) {
  // First network id lookup with direct buses connections
  for(uint8_t b = 0; b < MAX_BUSES; b++) {
    if(_buses[b].active) {
      if(network_id_equality(_buses[b].network_id, p.network_id)) {
        // First level connection detected
        // Send OSPREY Package as PJON packet to the directly connected PJON bus
        send_packet(_buses[b], p);
        return;
      } else {
        // Network id lookup in every router's connected bus list
        for(uint8_t r = 0; r < MAX_ROUTERS; r++) {
          for(uint8_t k = 0; k < MAX_KNOWN_NETWORKS; k++) {
            if(network_id_equality(_buses[b].routers[r].network_ids[k], p.network_id)) {
              // Second level connection detected
              // Send OSPREY Package as PJON packet to the router connected to the target PJON bus
              send_packet(_buses[b], p, _buses[b].routers[r].device_id);
              return;
            }
          }
        }
        // Network id lookup in all uknown networks
        for(uint8_t r = 0; r < MAX_ROUTERS; r++)
          send_packet(_buses[b], p, _buses[b].routers[r].device_id);
      }
    }
  }
};


void OSPREY::receive(uint8_t length, uint8_t *payload) {
  // Add package to the send list
  if(_router == true && payload[0] == ROUTER_IDENTIFIER)
    add(build_package(payload, length));
  // TODO - add receiver function for user
};


void OSPREY::send_packet(bus b, package p, uint8_t device_id = 0) {
  // Send OSPREY Package as PJON packet in the connected PJON bus
  int packet = b.transport.send(
    (device_id) ? device_id : p.device_id,
    build_content(p),
    p.length
  );

  if (packet == FAIL) {
    p.attempts++;
    // TODO - Maximum attempts check
  } else p.state = true;
};


char * OSPREY::build_content(package p) {
  char *memory = (char *) malloc(p.length] + 7);
  // TODO - Add memory fail
  // TODO - return;
  // TODO - CHECK JUMPS
  // if(jumps > MAX_JUMPS)
  //  send to sender id communication of failed package dispatch

  for(uint8_t i = 0; i < 4; i++)
    memory[i] = p.network_id[i];

  memory[4] = p.device_id;

  for(uint8_t i = 0; i < 4; i++)
    memory[i + 5] = p.sender_network_id[i];

  memory[9] = p.sender_device_id[i];
  memory[10] = generate_package_id();
  memory[11] = p.jumps++;

  for(uint8_t c = 0; c < p.length; c++)
    memory[c + 12] = p.content;

  return memory;
};


package OSPREY::build_package(uint8_t *content) {
  package p;

  for(uint8_t i = 0; i < 4; i++)
    p.network_id[i] = content[i];

  p.device_id = content[4];

  for(uint8_t i = 0; i < 4; i++)
    p.sender_network_id[i] = content[i + 5];

  p.sender_device_id = content[9];
  p.id = content[10];
  p.jumps = content[11]++;

  for(uint8_t c = 0; c < p.length; c++)
    p.content[i] = content[c + 12];

  return content;
};


uint8_t OSPREY::generate_package_id(uint8_t bus_id) {
  if (_package_id_source + 1 > _package_id_source)
    return _package_id_source++;
  else {
    _package_id_source = 0;
    return _package_id_source;
  }
};


boolean OSPREY::network_id_equality(uint8_t id_one[4], uint8_t id_two[4]) {
  for(var i = 0; i < 4; i++)
    if(id_one[i] != id_two[i])
      return false;

  return true;
};
