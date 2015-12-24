
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
  for(int p = 0; p < MAX_PACKAGES; p++) 
    _packages[i].active = false;

  for(int b = 0; b < MAX_BUSES; b++) 
    _buses[b].active = false;

  for(int r = 0; r < MAX_ROUTERS; r++) 
    _routers[r].active = false;

  for(int b = 0; b < MAX_BUSES; b++) 
    for(int n = 0; i < MAX_KNOWN_NETWORKS; n++)
      for(int i = 0; i < 4; i++)
        _buses[b].network_ids[n][[i] = 0;

  // TODO - Set receiver
  // TODO - Set error
};


void OSPREY::add_bus(Transport t, uint8_t device_id, uint8_t network_id[4], boolean router) {
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(!_buses[b].active) {
      _buses[b].transport = t;
      _buses[b].device_id = device_id;

      for(uint8_t n = 0; n < 4; n++)
        _buses[b].network_id[n] = network_id[n];
      
      _buses[b].router = router;
    }
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

  return p;
};


ping_response OSPREY::ping(uint8_t network_id[4], uint8_t device_id) {

};


void OSPREY::receive(bus b) {
  b.transport.receive();
};


void OSPREY::received(uint8_t length, uint8_t *payload) {
  // Add package to the send list
  for(uint8_t b = 0; b < MAX_BUSES; b++)
    if(network_id_equality([payload[0], payload[1], payload[2], payload[3]], _buses[b].network_id))
      if(payload[4] == _buses[b].device_id)
        this->_receiver(payload); // TODO - Filter out protocol data before passing

};


void free_package(uint8_t id) {

};


void OSPREY::send(package p) {
 
  // 1 First network id lookup with direct bus connections
  for(uint8_t b = 0; b < MAX_BUSES; b++) 
    if(_buses[b].active) 
      if(network_id_equality(_buses[b].network_id, p.network_id)) {
        // First level connection detected
        // Send OSPREY Package as PJON packet to the directly connected PJON bus 
        for(uint8_t r = 0; r < MAX_ROUTERS; r++)
          if(send_packet(_buses[b], p, _buses[b].routers[r].device_id))
            return;
      }

  // 2 Network id lookup in every router's connected bus list
  for(uint8_t b = 0; b < MAX_BUSES; b++) 
    for(uint8_t r = 0; r < MAX_ROUTERS; r++) 
      for(uint8_t k = 0; k < MAX_KNOWN_NETWORKS; k++)
        if(network_id_equality(_buses[b].routers[r].network_ids[k], p.network_id)) {
          // Second level connection detected
          // Send OSPREY Package as PJON packet to the router connected to the target PJON bus 
          send_packet(_buses[b], p, _buses[b].routers[r].device_id);
          return;
        }

  // 3 Network id lookup in all uknown networks
  for(uint8_t b = 0; b < MAX_BUSES; b++) 
    for(uint8_t r = 0; r < MAX_ROUTERS; r++) 
      if(send_packet(_buses[b], p, _buses[b].routers[r].device_id))
        return;   
};


void OSPREY::send_packet(bus b, package p, uint8_t device_id = 0) {           
  // Send OSPREY Package as PJON packet in the connected PJON bus 
  int packet = b.connector.send(
    (device_id) ? device_id : p.device_id, 
    build_content(p), 
    p.length
  );

  // TODO - Free the packet's content pointer when package is destroyed

  if (packet == FAIL) {
    p.attempts++;
    // TODO - Maximum attempts check
  } else p.state = DISPATCHED;
};


void OSPREY::update() {
  for(uint8_t p = 0; p < MAX_PACKAGES; p++) {
    if(_packages[p].state == TO_BE_DISPATCHED) {
      send(_packages[p]);
    }
  }
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

