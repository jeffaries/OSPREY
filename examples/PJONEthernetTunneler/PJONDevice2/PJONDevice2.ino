#include <PJON.h>

PJON<SoftwareBitBang> bus(45);

// Whether to reply to incoming messages (true) or to send 
// on own initiative (asynchronous, will cause collisions)
const bool relay = false;

void setup() {
  pinModeFast(13, OUTPUT);

  bus.set_pin(12);
  bus.set_receiver(receiver_function);
  bus.begin();
  if (!relay) bus.send(44, "B", 1, 20000);
}

void receiver_function(uint8_t id, uint8_t *payload, uint8_t length) {
 if((char)payload[0] == 'B') {
    static bool led_on = false;
    digitalWrite(13, led_on ? HIGH : LOW);
    led_on = !led_on;
    if (relay) bus.send(44, "B", 1);
  }
}

void loop() {
  bus.receive(1000);
  bus.update();
}
