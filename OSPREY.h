
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

#include "Transport.h"

#ifndef OSPREY_h
  #define OSPREY_h
  #if defined(ARDUINO) && (ARDUINO >= 100)
    #include "Arduino.h"
  #else
    #include "WProgram.h"
    #include "WConstants.h"
  #endif
#endif

// General constants

#define MAX_BUSES           10
#define MAX_JUMPS           10
#define MAX_KNOWN_NETWORKS  10
#define MAX_PACKAGES        10
#define MAX_ROUTERS         10
#define ROUTER_INFO_TIME    15000000 // 15 seconds

// Protocol symbols

#define INFO 100
#define PING 101
#define REQUEST 102
#define ASSERT  103

#define DESTINATION_BUS_UNREACHABLE     104
#define SOURCE_BUS_UNREACHABLE          105

#define DESTINATION_DEVICE_UNREACHABLE  106
#define SOURCE_DEVICE_UNREACHABLE       107

#define TIMEOUT                         108
#define JUMP_LIMIT                      109

#define PROHIBITED                      110

// Package states

#define TO_BE_DISPATCHED  111
#define DISPATCHED        112
#define DELIVERED         113


typedef struct {
  boolean    active;
  uint8_t    device_id;
  uint8_t    network_id[4];
  boolean    router;
  router     routers[MAX_ROUTERS];
  Transport  transport;
} bus;


typedef struct {
  boolean  active;
  uint8_t  device_id;
  uint8_t  network_id[4];
  uint8_t  network_ids[MAX_KNOWN_NETWORKS][4];
} router;

// Basic package strcture
typedef struct {
  uint8_t           attempts;
  char              *content;
  unsigned uint8_t  id;
  uint8_t           length;
  uint8_t           jumps;
  unsigned long     registration;
  uint8_t           recipient_device_id;
  uint8_t           recipient_network_id[4];
  uint8_t           sender_device_id;
  uint8_t           sender_network_id[4];
  uint8_t           state;
  unsigned long     timing;
} Package;

// Ping package structure
typedef struct: Package {
  uint8_t jumps[MAX_JUMPS][5];
} Ping;

// Info package structure
typedef struct: Package {
  uint8_t network_ids[MAX_KNOWN_NETWORKS][4];
} Info;

class OSPREY {
  public:
    OSPREY();
    
    void add_bus(Transport t, uint8_t device_id, uint8_t network_id[4], boolean router);
    void update();
    int  send_packet(bus b, Package p, uint8_t device_id);
  
  private:
    // TODO - Switch to linked lists
    bus      _buses[MAX_BUSES];
    Package  _packages[MAX_PACKAGES];
    router   _routers[MAX_ROUTERS];
    uint8_t  _package_id_source;
};
