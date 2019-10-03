
          /*\      __   __   __   __   __
          shs-    |  | |__  |__| |__| |__  \ /
         dM_d:    |__|  __| |    |  \ |__   |  0.1
        dL:KM     Configuration-less, plug-and-play,
       dM56Mh     dynamic networking over PJON.
      yM87MM:
      dgfi3h-
       NM*(Mm          /|  Copyright (c) 2014-2019,
   ___yM(U*MMo        /j|  Giovanni Blu Mitolo All rights reserved.
 _/OF/sMQWewrMNhfmmNNMN:|  Licensed under the Apache License, Version 2.0
|\_\+sMM":{rMNddmmNNMN:_|
       yMMMMso         \|  You may obtain a copy of the License at
       gtMfgm              http://www.apache.org/licenses/LICENSE-2.0
      mMA@Mf
      MMp';M
      ysM1MM:
       sMM3Mh     EXPERIMENTAL,
        dM6MN     USE AT YOUR OWN RISK.
         dMtd:
          \*/

#pragma once
#include "OSPREYDefines.h"

/* Reference to device */
struct Device_reference {
  uint8_t  packet_index = 0;
  uint32_t registration = 0;
  uint32_t rid          = 0;
  bool     state        = 0;
};

template<typename Strategy = SoftwareBitBang>
class OSPREYMaster : public PJON<Strategy> {
  public:
    bool debug = false;
    Device_reference ids[OSPREY_MAX_SLAVES];
    uint8_t required_config =
      PJON_TX_INFO_BIT | PJON_CRC_BIT | PJON_ACK_REQ_BIT | PJON_PORT_BIT;

    /* OSPREYMaster bus default initialization:
       State: Local (bus_id: 0.0.0.0)
       Acknowledge: true (Acknowledge is requested)
       device id: MASTER (254)
       Mode: PJON_HALF_DUPLEX
       Sender info: true (Sender info are included in the packet)
       Strategy: SoftwareBitBang */

    OSPREYMaster() : PJON<Strategy>(OSPREY_MASTER_ID) {
      set_default();
    };

    /* OSPREYMaster initialization passing bus and device id:
       uint8_t my_bus = {1, 1, 1, 1};
       OSPREYMaster master(my_bys); */

    OSPREYMaster(const uint8_t *b_id) : PJON<Strategy>(
      b_id,
      OSPREY_MASTER_ID
    ) {
      set_default();
    };

    /* Add a device reference: */

    bool add_id(uint8_t id, uint32_t rid, bool state) {
      if(!ids[id - 1].state && !ids[id - 1].rid) {
        ids[id - 1].rid = rid;
        ids[id - 1].state = state;
        return true;
      }
      return false;
    };

    /* Confirm a device id sending a repeated broadcast containing:
    OSPREY_ID_REQUEST - RID (4 byte random id) - DEVICE ID (the new assigned) */

    void approve_id(uint32_t rid) {
      uint8_t response[6];
      uint16_t state = reserve_id(rid);
      if(state == OSPREY_DEVICES_BUFFER_FULL) return;
      if(state == PJON_FAIL)
        return negate_id(PJON_NOT_ASSIGNED, rid);

      response[0] = OSPREY_ID_REQUEST;
      response[1] = (uint8_t)((uint32_t)(rid) >> 24);
      response[2] = (uint8_t)((uint32_t)(rid) >> 16);
      response[3] = (uint8_t)((uint32_t)(rid) >>  8);
      response[4] = (uint8_t)((uint32_t)(rid));
      response[5] = (uint8_t)(state);

      ids[response[5] - 1].packet_index = PJON<Strategy>::send_repeatedly(
        PJON_BROADCAST,
        PJON<Strategy>::bus_id,
        response,
        6,
        OSPREY_ID_REQUEST_INTERVAL,
        PJON<Strategy>::config | required_config,
        0,
        OSPREY_DYNAMIC_ADDRESSING_PORT
      );
    };

    /* Master begin function: */

    void begin() {
      PJON<Strategy>::begin();
      delete_id_reference();
      _list_time = PJON_MICROS();
      uint8_t request = OSPREY_ID_LIST;
      _list_id = PJON<Strategy>::send_repeatedly(
        PJON_BROADCAST,
        this->bus_id,
        &request,
        1,
        OSPREY_LIST_IDS_TIME,
        PJON<Strategy>::config | required_config,
        0,
        OSPREY_DYNAMIC_ADDRESSING_PORT
      );
    };

    /* Confirm device ID insertion in list: */

    bool confirm_id(uint32_t rid, uint8_t id) {
      if(ids[id - 1].rid == rid && !ids[id - 1].state) {
        if(
          (uint32_t)(PJON_MICROS() - ids[id - 1].registration) <
          OSPREY_ADDRESSING_TIMEOUT
        ) {
          ids[id - 1].state = true;
          PJON<Strategy>::remove(ids[id - 1].packet_index);
          return true;
        }
      }
      return false;
    };

    /* Count active slaves in buffer: */

    uint8_t count_slaves() {
      uint8_t result = 0;
      for(uint8_t i = 0; i < OSPREY_MAX_SLAVES; i++)
        if(ids[i].state) result++;
      return result;
    };

    /* Empty a single element or the whole buffer: */

    void delete_id_reference(uint8_t id = 0) {
      if(!id) {
        for(uint8_t i = 0; i < OSPREY_MAX_SLAVES; i++) {
          if(!ids[i].state && ids[i].rid)
            this->remove(ids[i].packet_index);
          ids[i].packet_index = 0;
          ids[i].registration = 0;
          ids[i].rid = 0;
          ids[i].state = false;
        }
      } else if(id > 0 && id < OSPREY_MAX_SLAVES) {
        if(!ids[id - 1].state && ids[id - 1].rid)
          this->remove(ids[id - 1].packet_index);
        ids[id - 1].packet_index = 0;
        ids[id - 1].registration = 0;
        ids[id - 1].rid   = 0;
        ids[id - 1].state = false;
      }
    };

    /* Master error handler: */

    void error(uint8_t code, uint16_t data) {
      _master_error(code, data, _custom_pointer);
      if(code == PJON_CONNECTION_LOST)
        delete_id_reference(PJON<Strategy>::packets[data].content[0]);
    };

    static void static_error_handler(uint8_t code, uint16_t data, void *cp) {
      ((OSPREYMaster<Strategy>*)cp)->error(code, data);
    };

    /* Filter addressing packets from receive callback: */

    void filter(
      uint8_t *payload,
      uint16_t length,
      const PJON_Packet_Info &packet_info
    ) {
      PJON_Packet_Info p_i;
      memcpy(&p_i, &packet_info, sizeof(PJON_Packet_Info));
      p_i.custom_pointer = _custom_pointer;
      if(!handle_addressing() || debug)
        _master_receiver(payload, length, p_i);
    };

    /* Handle addressing procedure if related: */

    bool handle_addressing() {
      bool filter = false;
      uint8_t overhead = PJON<Strategy>::packet_overhead(this->data[1]);
      uint8_t CRC_overhead = (this->data[1] & PJON_CRC_BIT) ? 4 : 1;

      if(
        (this->last_packet_info.header & PJON_PORT_BIT) &&
        (this->last_packet_info.header & PJON_TX_INFO_BIT) &&
        (this->last_packet_info.header & PJON_CRC_BIT) &&
        (this->last_packet_info.port == OSPREY_DYNAMIC_ADDRESSING_PORT)
      ) {
        filter = true;
        uint8_t request = this->data[overhead - CRC_overhead];
        uint32_t rid =
          (uint32_t)(this->data[(overhead - CRC_overhead) + 1]) << 24 |
          (uint32_t)(this->data[(overhead - CRC_overhead) + 2]) << 16 |
          (uint32_t)(this->data[(overhead - CRC_overhead) + 3]) <<  8 |
          (uint32_t)(this->data[(overhead - CRC_overhead) + 4]);

        if(request == OSPREY_ID_REQUEST)
          approve_id(rid);

        if(request == OSPREY_ID_CONFIRM)
          if(!confirm_id(rid, this->data[(overhead - CRC_overhead) + 5]))
            negate_id(this->last_packet_info.sender_id, rid);

        if(request == OSPREY_ID_REFRESH)
          if(!add_id(this->data[(overhead - CRC_overhead) + 5], rid, 1))
            negate_id(this->last_packet_info.sender_id, rid);

        if(request == OSPREY_ID_NEGATE)
          if(
            this->data[(overhead - CRC_overhead) + 5] ==
            this->last_packet_info.sender_id
          )
            if(rid == ids[this->last_packet_info.sender_id - 1].rid)
              if(
                PJONTools::bus_id_equality(
                  this->last_packet_info.sender_bus_id,
                  this->bus_id
                )
              ) delete_id_reference(this->last_packet_info.sender_id);
      }
      return filter;
    };

    /* Remove reserved id which expired (Remove never confirmed ids): */

    void free_reserved_ids_expired() {
      for(uint8_t i = 0; i < OSPREY_MAX_SLAVES; i++)
        if(!ids[i].state && ids[i].rid) {
          if(
            (uint32_t)(PJON_MICROS() - ids[i].registration) <
            OSPREY_ADDRESSING_TIMEOUT
          ) continue;
          else delete_id_reference(i + 1);
        }
    };

    /* Get device id from RID: */

    uint8_t get_id_from_rid(uint32_t rid) {
      for(uint8_t i = 0; i < OSPREY_MAX_SLAVES; i++)
        if(rid == ids[i].rid) return i + 1;
      return PJON_NOT_ASSIGNED;
    };

    /* Negate a device id request sending a packet to the device containing
       ID_NEGATE forcing the slave to make a new request. */

    void negate_id(uint8_t id, uint32_t rid) {
      uint8_t response[5] = {
        (uint8_t)OSPREY_ID_NEGATE,
        (uint8_t)((uint32_t)(rid) >> 24),
        (uint8_t)((uint32_t)(rid) >> 16),
        (uint8_t)((uint32_t)(rid) >>  8),
        (uint8_t)((uint32_t)(rid))
      };

      PJON<Strategy>::send_packet_blocking(
        id,
        PJON<Strategy>::bus_id,
        (char *)response,
        5,
        PJON<Strategy>::config | required_config,
        0,
        OSPREY_DYNAMIC_ADDRESSING_PORT
      );
    };

    /* Reserve a device id and wait for its confirmation: */

    uint16_t reserve_id(uint32_t rid) {
      for(uint8_t i = 0; i < OSPREY_MAX_SLAVES; i++)
        if((ids[i].rid == rid) || (!ids[i].state && !ids[i].rid)) {
          ids[i].registration = PJON_MICROS();
          ids[i].rid = rid;
          ids[i].state = false;
          return i + 1;
        }
      error(OSPREY_DEVICES_BUFFER_FULL, OSPREY_MAX_SLAVES);
      return OSPREY_DEVICES_BUFFER_FULL;
    };

/* Master receive function: */

    uint16_t receive() {
      return PJON<Strategy>::receive();
    };

    /* Try to receive a packet repeatedly with a maximum duration: */

    uint16_t receive(uint32_t duration) {
      uint32_t time = PJON_MICROS();
      while((uint32_t)(PJON_MICROS() - time) <= duration)
        if(receive() == PJON_ACK) return PJON_ACK;
      return PJON_FAIL;
    };

    /* Static receiver hander: */

    static void static_receiver_handler(
      uint8_t *payload,
      uint16_t length,
      const PJON_Packet_Info &packet_info
    ) {
      (
        (OSPREYMaster<Strategy>*)packet_info.custom_pointer
      )->filter(payload, length, packet_info);
    };

    /* Set custom pointer: */

    void set_custom_pointer(void *p) {
      _custom_pointer = p;
    };

    /* Set default configuration: */

    void set_default() {
      PJON<Strategy>::set_default();
      PJON<Strategy>::set_custom_pointer(this);
      PJON<Strategy>::set_error(static_error_handler);
      PJON<Strategy>::set_receiver(static_receiver_handler);
      delete_id_reference();
      this->include_port(true);
    };

    /* Master receiver function setter: */

    void set_receiver(PJON_Receiver r) {
      _master_receiver = r;
    };

    /* Master error receiver function: */

    void set_error(PJON_Error e) {
      _master_error = e;
    };

    /* Master packet handling update: */

    uint8_t update() {
      if(
        (_list_id != PJON_MAX_PACKETS) &&
        (OSPREY_ADDRESSING_TIMEOUT < (uint32_t)(PJON_MICROS() - _list_time))
      ) {
        PJON<Strategy>::remove(_list_id);
        _list_id = PJON_MAX_PACKETS;
      }
      free_reserved_ids_expired();
      return PJON<Strategy>::update();
    };

  private:
    uint16_t        _list_id = PJON_MAX_PACKETS;
    uint32_t        _list_time;
    void           *_custom_pointer;
    PJON_Receiver   _master_receiver;
    PJON_Error      _master_error;
};
