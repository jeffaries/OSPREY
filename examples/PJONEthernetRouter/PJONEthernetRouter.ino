/*
 * PJONEthernetRouter
 * This sketch is meant for an arduino with an Ethernet shield, and also connected to one or more PJON media.
 * It is meant to be run in multiple instances, with a routing table that will pick up all packages to destined for a bus
 * that the routing table indocates that this instance can deliver to. The instance responsible for the target bus on the
 * the other end of the Ethernet connection will deliver it to a PJON media on its side so.
 * 
 * Arduino A - <PJON wired or wireless> - PJONEthernetRouterA - <TCP/IP> - PJONEthernetRouterB - <PJON wired or wireless> - Arduino B
 * 
 * The Ethernet TCP/IP connections may cross network segments or go through the Internet as long as a route is open.
 */

#include <Arduino.h>
#include <OSPREY.h>
#include <EthernetLink.h>
#include <PJONLink.h>

// Ethernet settings
byte REMOTEIP1[] = { 192,168,1,10 };
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
byte ip[] = { 192, 168, 1, 11 };    
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Device ID
#define REMOTEID1 45
#define LOCALID 44

// PJON settings
#define PJONPIN 7
uint8_t LOCALBUS[] =  { 0xAA, 0xBB, 0xCC, 0xDD };
uint8_t REMOTEBUS[] = { 0xBB, 0xCC, 0xDD, 0xEE };

OSPREY osprey;
PJONLink pjonLink<SoftwareBitBang>(LOCALID);
EthernetLink ethernetLink(LOCALID);

void setup() {
  Ethernet.begin(mac, ip); //, gateway, subnet);
  pjonLink.set_pin(PJONPIN);
  ethernetLink.add_node(REMOTEID1, REMOTEIP1);
  osprey.add_bus(ethernetLink, REMOTEBUS, true);
  osprey.add_bus(pjonLink, LOCALBUS, true);
}

void loop() {
  osprey.receive(1000);
  osprey.update();  
}
