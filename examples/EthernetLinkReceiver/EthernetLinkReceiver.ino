#include <Arduino.h>
#include <EthernetLink.h>

// Ethernet settings
byte REMOTEIP1[] = {192,168,1,110 };
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
byte ip[] = { 192, 168, 1, 111 };    
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Device ID
#define LOCALID 45
#define REMOTEID1 44

EthernetLink ethernetLink(LOCALID);

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome to EthernetLinkReceiver");
  pinMode(13, OUTPUT);
  
  Ethernet.begin(mac, ip, gateway, subnet);
  ethernetLink.add_node(REMOTEID1, REMOTEIP1);
  ethernetLink.set_receiver(receiver_function);
  ethernetLink.set_error(error_function);
  ethernetLink.start_listening();
}

void loop() {  
  ethernetLink.update();
  ethernetLink.receive(1000);
}

void receiver_function(uint8_t id, uint8_t *payload, uint8_t length) {
  ethernetLink.send(REMOTEID1, "FINE, THANK YOU!", 17); // Including null-term, so safe to print
  Serial.print("Received request: "); Serial.println((const char *) payload);}

void error_function(uint8_t code, uint8_t data) {
  Serial.print("Got error code "); Serial.println(code);
}
