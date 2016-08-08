#pragma once

#include <PJONLink.h>

struct PJONSoftwareBitBangLink : public PJONLink {
  PJON<SoftwareBitBang> bus;

  PJONSoftwareBitBangLink(uint8_t device_id) {
    bus.set_id(device_id);
  };

  PJONSoftwareBitBangLink(uint8_t device_id, const uint8_t *bus_id) {
    bus.set_id(device_id);
    bus.copy_bus_id(bus.bus_id, bus_id);
  };

  uint16_t receive() { return bus.receive(); };
  uint16_t receive(uint32_t duration) { return bus.receive(duration); };

  void update() { bus.update(); }
  uint16_t dispatch(uint8_t id, uint8_t *b_id, const char *packet, uint8_t length, uint32_t timing, uint8_t header = 0) {
    dispatch(id, b_id, packet, length, timing, header);
  }

  uint16_t send_string(uint8_t id, char *string, uint8_t length, uint8_t header = 0) {
    bus.send_string(id, string, length, header);
  }

  const PacketInfo &get_last_packet_info() const { return bus.last_packet_info; };

  const uint8_t device_id() const { return bus.device_id(); };
  const uint8_t *bus_id() const { return bus.bus_id; };
  void set_receiver(receiver r) { bus.set_receiver(r); };
};
