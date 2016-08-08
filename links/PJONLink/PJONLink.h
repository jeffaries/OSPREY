#pragma once

#include <PJON.h>

struct PJONLink  {
  virtual const PacketInfo &get_last_packet_info() const = 0;

  static boolean bus_id_equality(const uint8_t *name_one, const uint8_t *name_two) {
    for(uint8_t i = 0; i < 4; i++) if(name_one[i] != name_two[i]) return false;
    return true;
  };

  static void copy_bus_id(uint8_t *target, const uint8_t *source) { memcpy(target, source, 4); };

  virtual uint16_t dispatch(
    uint8_t id,
    uint8_t *b_id,
    const char *packet,
    uint8_t length,
    uint32_t timing,
    uint8_t header = 0
  ) = 0;

  virtual const uint8_t get_id() const = 0;
  virtual const uint8_t *get_bus_id() const = 0;

  virtual uint16_t receive() = 0;
  virtual uint16_t receive(uint32_t duration) = 0;

  virtual uint16_t send_string(uint8_t id, char *string, uint8_t length, uint8_t header = 0) = 0;
  virtual void set_receiver(receiver r) = 0;

  virtual void update() = 0;
};
