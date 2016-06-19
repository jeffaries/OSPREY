
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

#ifndef OSPREY_h
  #define OSPREY_h
  #include "Arduino.h"
  #include <Link.h>
  #include <PJON.h>

  /* Buses buffer length */
  #define MAX_BUSES              5
  /* Maximum hops before timeout */
  #define MAX_HOPS               10
  /* Known buses buffer length */
  #define MAX_KNOWN_BUSES        5
  /* Known devices buffer length */
  #define MAX_KNOWN_DEVICES      5
  /* Max package references kept in memory */
  #define MAX_PACKAGE_REFERENCES 10
  /* Interval between every info packet sending in microseconds */
  #define ROUTER_INFO_TIME       15000000

  // Protocol symbols
  #define ASSERT             100
  #define INFO               102
  #define INTERNET_ASSERT    103
  #define INTERNET_REQUEST   104
  #define PING               105
  #define REQUEST            106

  // Errors
  #define BUS_UNREACHABLE    256
  #define DEVICE_UNREACHABLE 257
  #define TIMEOUT            258
  #define HOPS_LIMIT         259
  #define PROHIBITED         260

  // States
  #define DISPATCHED         265
  #define DELIVERED          266

  // Device structure
  typedef struct {
    boolean active;
    boolean router;
    uint8_t id;
    uint8_t bus_id[4];
    uint8_t known_bus_ids[MAX_KNOWN_BUSES][4];
    uint8_t name[5];
  } Device;

  // Bus structure
  typedef struct {
    boolean active;                           // Bus activity state boolean
    Device  known_devices[MAX_KNOWN_DEVICES]; // Known devices in this bus
    Link  link;                             // Link (PJON or PJON_ASK instance)
  } Bus;

  // Package reference
  typedef struct {
    uint8_t   bus_id[4];     // The bus where the packet is dispatched
    uint8_t   packet_index;  // Its packet id in the PJON packet buffer
    uint16_t  package_id;    // Its assigned package id from source
  } Package_reference;

  typedef void (* routing_handler)(
    uint8_t *recipient_bus_id,
    uint8_t recipient_device_id,
    uint8_t *sender_bus_id,
    uint8_t sender_device_id,
    uint8_t type,
    uint8_t hops,
    uint16_t id,
    uint8_t *content,
    uint8_t length
  );

  static void dummy_handler(
    uint8_t *recipient_bus_id,
    uint8_t recipient_device_id,
    uint8_t *sender_bus_id,
    uint8_t sender_device_id,
    uint8_t type,
    uint8_t hops,
    uint16_t id,
    uint8_t *content,
    uint8_t length
  ) {};

  class OSPREY {
    public:
      OSPREY();
      uint8_t add_bus(Link link, uint8_t *bus_id, uint8_t device_id, boolean router);
      void add_package_reference(uint8_t bus_id[4], uint16_t package_id, uint8_t packet_index);
      boolean bus_id_equality(uint8_t *id_one, uint8_t *id_two);
      uint8_t count_active_buses();
      uint16_t generate_package_id();

      void handle_packet(uint8_t bus_id[4], uint8_t packet_index, uint8_t state);

      void receive(uint32_t duration);
      void received(uint8_t id, uint8_t *payload, uint8_t length);

      void remove_package_reference(uint8_t bus_id[4], uint8_t packet_index);

      uint16_t send(
        Link       link,
        uint8_t    *bus_id,
        uint8_t    device_id,
        uint8_t    type,
        uint8_t    hops,
        uint16_t   package_id,
        char       *content,
        uint8_t    length
      );

      uint16_t send(uint8_t bus, uint8_t device_id, char *payload, uint8_t length);
      uint16_t send(uint8_t *bus_id, uint8_t device_id, uint8_t type, uint8_t hops, char *content, uint8_t length);
      uint16_t send(uint8_t *bus_id, uint8_t device_id, uint8_t type, char *content, uint8_t length);

      void set_routing_handler(routing_handler h);
      void update();

      Bus buses[MAX_BUSES];
      Package_reference package_references[MAX_PACKAGE_REFERENCES];
    private:
      uint8_t  _package_id_source;
      routing_handler  _routing_handler;
  };

#endif
