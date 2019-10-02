
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

template<typename Strategy = SoftwareBitBang>
class OSPREYSlave : public PJON<Strategy> {
  public:
    bool connected = false;
    uint8_t required_config =
      PJON_ACK_REQ_BIT | PJON_TX_INFO_BIT | PJON_CRC_BIT | PJON_PORT_BIT;

    /* OSPREYSlave bus default initialization:
       State: Local (bus_id: 0.0.0.0)
       Acknowledge: true (Acknowledge is requested)
       device id: PJON_NOT_ASSIGNED (255)
       Mode: PJON_HALF_DUPLEX
       Sender info: included
       Strategy: SoftwareBitBang

       OSPREYSlave initialization:
       OSPREYSlave<SoftwareBitBang> bus; */

    OSPREYSlave() : PJON<Strategy>(PJON_NOT_ASSIGNED) {
      set_default();
    };

    /* OSPREYSlave initialization passing bus and device id:
       uint8_t my_bus = {1, 1, 1, 1};
       OSPREYSlave<SoftwareBitBang> bus(my_bys); */

    OSPREYSlave(const uint8_t *bi) : PJON<Strategy>(bi, PJON_NOT_ASSIGNED) {
      set_default();
    };

	/* Define, prior to call to Begin(), the universal unique id of the slave. */

	void set_uuid(uint32_t rid){
		if(!_rid) _rid = rid;
	}

/* Acquire id in master-slave configuration: */

bool request_id() {
  if(!_rid) generate_rid();
      char response[5];
      response[0] = OSPREY_ID_REQUEST;
      response[1] = (uint8_t)((uint32_t)(_rid) >> 24);
      response[2] = (uint8_t)((uint32_t)(_rid) >> 16);
      response[3] = (uint8_t)((uint32_t)(_rid) >>  8);
      response[4] = (uint8_t)((uint32_t)(_rid));

      if(this->send_packet_blocking(
        OSPREY_MASTER_ID,
        this->bus_id,
        response,
        5,
        this->config | required_config,
        0,
        OSPREY_DYNAMIC_ADDRESSING_PORT
      ) == PJON_ACK) return true;

      error(OSPREY_ID_ACQUISITION_FAIL, OSPREY_ID_REQUEST);
      return false;
    };

    /* Begin function to be called in setup: */

    void begin() {
      PJON<Strategy>::begin();
    };

    /* Release device id (Master-slave only): */

    bool discard_device_id() {
      char request[6] = {
        OSPREY_ID_NEGATE,
        (uint8_t)((uint32_t)(_rid) >> 24),
        (uint8_t)((uint32_t)(_rid) >> 16),
        (uint8_t)((uint32_t)(_rid) >>  8),
        (uint8_t)((uint32_t)(_rid)),
        this->_device_id
      };

      if(this->send_packet_blocking(
        OSPREY_MASTER_ID,
        this->bus_id,
        request,
        6,
        this->config | required_config,
        0,
        OSPREY_DYNAMIC_ADDRESSING_PORT
      ) == PJON_ACK) {
        this->_device_id = PJON_NOT_ASSIGNED;
        return true;
      }
      error(OSPREY_ID_ACQUISITION_FAIL, OSPREY_ID_NEGATE);
      return false;
    };

    /* Error callback: */

    void error(uint8_t code, uint16_t data) {
      _slave_error(code, data, _custom_pointer);
    };

    /* Filter incoming addressing packets callback: */

    void filter(
      uint8_t *payload,
      uint16_t length,
      const PJON_Packet_Info &packet_info
    ) {
      if(!handle_addressing()) {
         PJON_Packet_Info p_i;
         memcpy(&p_i, &packet_info, sizeof(PJON_Packet_Info));
         p_i.custom_pointer = _custom_pointer;
        _slave_receiver(payload, length, p_i);
      }
    };

    /* Generate a new device rid: */

    void generate_rid() {
      _rid = (
        (uint32_t)(PJON_ANALOG_READ(this->random_seed)) ^
        (uint32_t)(PJON_MICROS())
      ) ^ _rid ^ _last_request_time;
    };

    /* Get device rid: */

    uint32_t get_rid() {
      return _rid;
    };

    /* Handle dynamic addressing requests and responses: */

    bool handle_addressing() {
      if( // Detect mult-master dynamic addressing
        (this->last_packet_info.header & PJON_PORT_BIT) &&
        (this->last_packet_info.header & PJON_TX_INFO_BIT) &&
        (this->last_packet_info.header & PJON_CRC_BIT) &&
        (this->last_packet_info.port == OSPREY_DYNAMIC_ADDRESSING_PORT) &&
        (
          (this->last_packet_info.sender_id == PJON_NOT_ASSIGNED) ||
          (this->last_packet_info.sender_id == this->_device_id)
        )
      ) return true;

      if( // Handle master-slave dynamic addressing
        (this->last_packet_info.header & PJON_PORT_BIT) &&
        (this->last_packet_info.header & PJON_TX_INFO_BIT) &&
        (this->last_packet_info.header & PJON_CRC_BIT) &&
        (this->last_packet_info.port == OSPREY_DYNAMIC_ADDRESSING_PORT) &&
        (this->_device_id != OSPREY_MASTER_ID) &&
        (this->last_packet_info.sender_id == OSPREY_MASTER_ID)
      ) {

        uint8_t overhead =
          this->packet_overhead(this->last_packet_info.header);
        uint8_t CRC_overhead =
          (this->last_packet_info.header & PJON_CRC_BIT) ? 4 : 1;
        uint8_t rid[4] = {
          (uint8_t)((uint32_t)(_rid) >> 24),
          (uint8_t)((uint32_t)(_rid) >> 16),
          (uint8_t)((uint32_t)(_rid) >>  8),
          (uint8_t)((uint32_t)(_rid))
        };
        char response[6];
        response[1] = rid[0];
        response[2] = rid[1];
        response[3] = rid[2];
        response[4] = rid[3];

        if(this->data[overhead - CRC_overhead] == OSPREY_ID_REQUEST)
          if(
            PJONTools::bus_id_equality(
              this->data + ((overhead - CRC_overhead) + 1),
              rid
            )
          ) {
            response[0] = OSPREY_ID_CONFIRM;
            response[5] = this->data[(overhead - CRC_overhead) + 5];
            this->set_id(response[5]);
            if(this->send_packet_blocking(
              OSPREY_MASTER_ID,
              this->bus_id,
              response,
              6,
              this->config | required_config,
              0,
              OSPREY_DYNAMIC_ADDRESSING_PORT
            ) != PJON_ACK) {
              this->set_id(PJON_NOT_ASSIGNED);
              connected = false;
              error(OSPREY_ID_ACQUISITION_FAIL, OSPREY_ID_CONFIRM);
            } else connected = true;
          }

        if(this->data[overhead - CRC_overhead] == OSPREY_ID_NEGATE)
          if(
            PJONTools::bus_id_equality(
              this->data + ((overhead - CRC_overhead) + 1),
              rid
            ) && this->_device_id == this->data[0]
          ) request_id();

        if(this->data[overhead - CRC_overhead] == OSPREY_ID_LIST) {
          if(this->_device_id != PJON_NOT_ASSIGNED) {
            if(
              (uint32_t)(PJON_MICROS() - _last_request_time) >
              (OSPREY_ADDRESSING_TIMEOUT * 1.125)
            ) {
              _last_request_time = PJON_MICROS();
              response[0] = OSPREY_ID_REFRESH;
              response[5] = this->_device_id;
              this->send(
                OSPREY_MASTER_ID,
                this->bus_id,
                response,
                6,
                this->config | required_config,
                0,
                OSPREY_DYNAMIC_ADDRESSING_PORT
              );
            }
          } else if(
            (uint32_t)(PJON_MICROS() - _last_request_time) >
            (OSPREY_ADDRESSING_TIMEOUT * 1.125)
          ) {
            _last_request_time = PJON_MICROS();
            request_id();
          }
        }
        return true;
      }
      return false;
    };

    /* Slave receive function: */

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

    /* Set custom pointer: */

    void set_custom_pointer(void *p) {
      _custom_pointer = p;
    }

    /* Set default configuration: */

    void set_default() {
      PJON<Strategy>::set_default();
      PJON<Strategy>::set_custom_pointer(this);
      PJON<Strategy>::set_receiver(static_receiver_handler);
      PJON<Strategy>::set_error(static_error_handler);
      this->include_port(true);
    };

    /* Slave receiver function setter: */

    void set_receiver(PJON_Receiver r) {
      _slave_receiver = r;
    };

    /* Master error receiver function: */

    void set_error(PJON_Error e) {
      _slave_error = e;
    };

    /* Static receiver hander: */

    static void static_receiver_handler(
      uint8_t *payload,
      uint16_t length,
      const PJON_Packet_Info &packet_info
    ) {
      (
        (OSPREYSlave<Strategy>*)packet_info.custom_pointer
      )->filter(payload, length, packet_info);
    };

    /* Static error hander: */

    static void static_error_handler(
      uint8_t code,
      uint16_t data,
      void *custom_pointer
    ) {
      ((OSPREYSlave<Strategy>*)custom_pointer)->error(code, data);
    };

    /* Slave packet handling update: */

    uint8_t update() {
      return PJON<Strategy>::update();
    };

  private:
    void          *_custom_pointer;
    uint32_t      _last_request_time;
    uint32_t      _rid;
    PJON_Error    _slave_error;
    PJON_Receiver _slave_receiver;
};
