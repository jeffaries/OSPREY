/*
 * PJONEthernetTunneler
 * This sketch is meant for an arduino with an Ethernet shield, and also connected to a PJON media.
 * It is meant to be run in two instances, and will send incoming PJON messages through an ethernet tunnel to another instance 
 * of this sketch which will again deliver it to a PJON media on its side.
 * 
 * Arduino A - <PJON wired or wireless> - PJONEthernetTunnelB - <TCP/IP> - PJONEthernetTunnelA - <PJON wired or wireless> - Arduino B
 *  (ID 45)                                    (ID 44)                           (ID 45)                                     (ID 44)
 * 
 * So device A can send a message to device B, and the message will be delivered to device B on the remote media.
 * And also in the reverse direction. The device acts as a stand-in Proxy, connecting two buses with a fixed recipient on each side.
 * 
 * The Ethernet TCP/IP connections may cross network segments or go through the Internet as long as a route is open.
 */

#include <Arduino.h>
#include <PJON.h>
#include <EthernetLink.h>

// Basic Ethernet settings
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Devices
uint8_t this_device = 0; // 0 or 1, chooses which PJON/Ethernet device this is
uint8_t ids[] = { 44, 45 };
byte ip[][4] = { { 192, 168, 1, 10 }, { 192, 168, 1, 11 } };
byte mac[][6] = { { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }, { 0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xED } };  

// PJON settings
#define PJONPIN 7

PJON<SoftwareBitBang> pjon(ids[this_device]);
EthernetLink ethernetLink(ids[this_device]);

void setup() {
  Serial.begin(115200);
  Serial.print("Welcome to PJONEthernetTunneler, role ");
  Serial.println(this_device);
  
  Ethernet.begin(mac[this_device], ip[this_device], gateway, subnet);
  delay(1000);
  ethernetLink.set_receiver(ethernet_receiver);
  ethernetLink.add_node(ids[1-this_device], ip[1-this_device]);
  ethernetLink.start_listening();
  
  pjon.set_pin(PJONPIN);
  pjon.set_receiver(pjon_receiver);
}

void loop() {
  pjon.receive(1000);
  pjon.update();

  ethernetLink.receive(1000);
  ethernetLink.update();
}

void ethernet_receiver(uint8_t id, uint8_t *payload, uint8_t length) {
  Serial.print("Forwarded Ethernet message from E_ID "); Serial.print(id);
  Serial.print(" to P_ID device "); Serial.print(ids[1-this_device]);
  print_payload(payload, length);
  pjon.send(ids[1-this_device], (char *)payload, length);
}

void pjon_receiver(uint8_t id, uint8_t *payload, uint8_t length) {
  Serial.print("Forwarded PJON message from P_ID "); Serial.print(id);
  Serial.print(" to E_ID device "); Serial.print(ids[1-this_device]);
  print_payload(payload, length);
  ethernetLink.send(ids[1-this_device], (char *)payload, length, 10000000);
}

void print_payload(uint8_t *payload, uint8_t length) {
  char buf[length+1];
  memcpy(buf, payload, length);
  buf[length] = 0;
  Serial.print(", data:'"); Serial.print(buf); Serial.println("'.");
}

