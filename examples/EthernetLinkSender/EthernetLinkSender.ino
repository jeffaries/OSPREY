#include <Arduino.h>
#include <EthernetLink.h>

// Ethernet settings
byte REMOTEIP1[] = { 192,168,1,111 };
byte mac[] = { 0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
byte ip[] = { 192, 168, 1, 110 };    
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Device ID
#define LOCALID 44
#define REMOTEID1 45

EthernetLink ethernetLink(LOCALID);

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome to EthernetLinkSender");
  pinMode(13, OUTPUT);
  
  Ethernet.begin(mac, ip, gateway, subnet);
  ethernetLink.add_node(REMOTEID1, REMOTEIP1);
  ethernetLink.set_receiver(receiver_function);
  ethernetLink.set_error(error_function);
  ethernetLink.start_listening();
}

void loop() {
  // Send a package every few seconds
  static unsigned long last = millis();
  unsigned long now = millis();
  if (last + 5000 <= now) {
    last = now;
    ethernetLink.send(REMOTEID1, "HOW DO YOU DO?", 15); // Including null-term, so safe to print
  }
  
  ethernetLink.update();
  ethernetLink.receive(1000);
}

void receiver_function(uint8_t id, uint8_t *payload, uint8_t length) {
  Serial.print("Received reply: "); Serial.println((const char *) payload);  
}

void error_function(uint8_t code, uint8_t data) {
  Serial.print("Got error code: "); Serial.println(code);
}
