#pragma once

#include "links/Link.h"

  struct PJONOverSamplingLink : public Link {
  PJON<OverSampling> bus;

  PJONOverSamplingLink(uint8_t device_id) {
    bus.set_id(device_id);
  };

  PJONOverSamplingLink(const uint8_t *bus_id, uint8_t device_id) {
    bus.set_id(device_id);
    bus.copy_bus_id(bus.bus_id, bus_id);
  };

  uint16_t receive() { return bus.receive(); };
  uint16_t receive(uint32_t duration) { return bus.receive(duration); };

  void update() { bus.update(); };

  uint16_t dispatch(uint8_t id, uint8_t *b_id, const char *packet, uint8_t length, uint32_t timing, uint8_t header = 0) {
    dispatch(id, b_id, packet, length, timing, header);
  };

  uint16_t send_string(uint8_t id, char *string, uint8_t length, uint8_t header = 0) {
    bus.send_string(id, string, length, header);
  };

  const PacketInfo &get_last_packet_info() const { return bus.last_packet_info; };

  uint8_t device_id() { return bus.device_id(); };
  uint8_t *bus_id() { return bus.bus_id; };

  void set_receiver(receiver r) { bus.set_receiver(r); };
  void set_error(error r) { bus.set_error(r); };
};
