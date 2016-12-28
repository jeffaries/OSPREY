
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

#ifndef OSPREY_h
  #define OSPREY_h
  #include <Arduino.h>
  #include <PJON.h>

  #include "links/Link.h"
  #include "links/PJONLink/PJONOverSamplingLink.h"
  #include "links/PJONLink/PJONSoftwareBitBangLink.h"
  #include "links/PJONLink/PJONThroughHardwareSerialLink.h"

  /* Buses buffer length */
  #define MAX_BUSES              3
  /* Maximum hops before timeout */
  #define MAX_HOPS               10
  /* Known buses buffer length */
  #define MAX_KNOWN_BUSES        3
  /* Known devices buffer length */
  #define MAX_KNOWN_DEVICES      5
  /* Interval between every info packet sending in microseconds */
  #define ROUTER_INFO_TIME       15000000

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
    uint8_t bus_id[4];
    uint8_t id;
    boolean router;
  } Device;

  // Bus structure
  typedef struct {
    boolean active;                           // Bus activity state boolean
    Device  known_devices[MAX_KNOWN_DEVICES]; // Known devices in this bus
    Link  *link;                              // Link (PJON or PJON_ASK instance)
  } Bus;

  typedef void (* routing_handler) (
    uint8_t  *recipient_bus_id,
    uint8_t  recipient_device_id,
    uint8_t  *sender_bus_id,
    uint8_t  sender_device_id,
    uint8_t  *content,
    uint16_t length,
    uint16_t packet_id,
    uint8_t  hops
  );

  static void dummy_handler(
    uint8_t  *recipient_bus_id,
    uint8_t  recipient_device_id,
    uint8_t  *sender_bus_id,
    uint8_t  sender_device_id,
    uint16_t id,
    uint8_t  *content,
    uint16_t length
    uint8_t  hops,
  ) {};

  class OSPREY {
    public:
      OSPREY();
      uint8_t add_bus(Link *link);
      boolean bus_id_equality(const uint8_t *id_one, const uint8_t *id_two);
      uint8_t count_active_buses();
      uint16_t find_bus(const uint8_t *bus_id, uint8_t id);
      uint16_t generate_package_id();

      void handle_packet(const uint8_t *bus_id, uint8_t packet_index, uint8_t state);

      void receive(uint32_t duration);
      void received(uint8_t *payload, uint16_t length, const PacketInfo &packet_info);

      uint16_t send(
        const uint8_t *recipient_bus_id,
        uint8_t       recipient_device_id,
        const uint8_t *sender_bus_id,
        uint8_t       sender_device_id,
        const char    *content,
        uint16_t      length,
        uint16_t      header = NOT_ASSIGNED,
        uint16_t      packet_id = 0,
        uint8_t       hops = 0
      );

      uint16_t dispatch(
        uint8_t bus_index,
        const uint8_t *recipient_bus_id,
        uint8_t       recipient_device_id,
        const uint8_t *sender_bus_id,
        uint8_t       sender_device_id,
        const char    *content,
        uint16_t      length,
        uint16_t      header = NOT_ASSIGNED,
        uint16_t      packet_id = 0,
        uint8_t       hops = 0
      );

      void update();

      Bus *buses[MAX_BUSES];
      Package_reference package_references[MAX_PACKAGE_REFERENCES];
    private:
      uint8_t _package_id_source;
  };

#endif
