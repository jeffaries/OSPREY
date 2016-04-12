
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
             \/     "This product includes OSPREY developed by Giovanni Blu Mitolo."

  - If a product includes / incorporates more than one product
    developed by the author, acknowledgements can be merged:
    "This product includes OSPREY and PJON software developed by Giovanni Blu Mitolo."

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

#ifndef OSPREY_h
  #define OSPREY_h

  #include "Arduino.h"
  #include "includes/Transport.h"
#endif

// General constants

#define BUS_ID_LENGTH        4
/* Use 4 bytes to comply to the OSPREY network Standard and be able to connect
   your network to other people ones. */

#define MAX_BUSES           10
#define MAX_HOPS            10
#define MAX_KNOWN_BUSES     10
#define MAX_KNOWN_DEVICES   10
#define MAX_PACKAGE_LENGTH  50

#define MAX_ROUTERS         10
#define ROUTER_INFO_TIME    15000000 // 15 seconds

// Protocol symbols
#define ASSERT            100
#define INFO              101
#define INTERNET_ASSERT   102
#define INTERNET_REQUEST  103
#define PING              104
#define REQUEST           105

// Package states
#define FREE              110
#define TO_BE_DISPATCHED  111
#define DISPATCHED        112
#define DELIVERED         113

// Errors
#define DESTINATION_BUS_UNREACHABLE     115
#define SOURCE_BUS_UNREACHABLE          116
#define DESTINATION_DEVICE_UNREACHABLE  117
#define SOURCE_DEVICE_UNREACHABLE       118
#define TIMEOUT                         119
#define HOPS_LIMIT                      120
#define PROHIBITED                      121
#define PACKAGE_BUFFER_FULL             122

// Basic Bus structure
typedef struct {
  boolean  active;                          // Bus activity state boolean
  uint8_t  id[BUS_ID_LENGTH];               // n byte bus id
  Device   device;                          // Device object
  Device   known_devices[MAX_KNOW_DEVICES]; // Known devices in this bus
  Link     link;                            // Link (PJON or PJON_ASK instance)
} Bus;

// Basic Device structure
typedef struct {
  boolean  active; // Device activity state boolean
  boolean  router; // Router functionality state boolean
  uint8_t  id;     // Device id on its PJON bus
  uint8_t  known_bus_ids[MAX_KNOWN_BUSES][BUS_ID_LENGTH]; // Known bus_ids
} Device;

// Basic package structure
typedef struct Package {
  uint8_t           attempts;
  uint8_t           *content;
  uint8_t           length;
  unsigned long     registration;
  uint8_t           state;
  unsigned long     timing;
} Package;

// Ping package structure
typedef struct: Package {
  uint8_t hops[MAX_HOPS][BUS_ID_LENGTH + 1];
} Ping;

// Info package structure
typedef struct: Package {
  uint8_t known_bus_ids[MAX_KNOWN_BUSES][BUS_ID_LENGTH];
} Info;

class OSPREY {
  public:
    OSPREY();
    uint8_t add_bus(Link l, uint8_t bus_id[BUS_ID_LENGTH], boolean router);
    uint8_t add_package(Package p);

    boolean bus_id_equality(uint8_t *id_one, uint8_t *id_two);

    void free_package(uint8_t id);

    uint16_t generate_package_id();

    void receive(unsigned long duration);
    void received(uint8_t length, uint8_t *payload);

    uint8_t send(Package p);
    void update();

    Package  packages[MAX_PACKAGES];
    Bus      buses[MAX_BUSES];
  private:
    uint8_t  _package_id_source;
    uint8_t  _known_bus_ids[MAX_KNOWN_BUSES][BUS_ID_LENGTH];
};
