#pragma once

#include <PJON.h>
#include <Link.h>

struct PJONLink : Link  {
  virtual const PacketInfo &get_last_packet_info() const = 0;

  static boolean bus_id_equality(const uint8_t *name_one, const uint8_t *name_two) {
    for(uint8_t i = 0; i < 4; i++) if(name_one[i] != name_two[i]) return false;
    return true;
  };

  static void copy_bus_id(uint8_t *target, const uint8_t *source) { memcpy(target, source, 4); };
};
