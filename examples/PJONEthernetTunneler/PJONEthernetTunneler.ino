/*
 * PJONEthernetTunneler
 * This sketch is meant for an arduino with an Ethernet shield, and also connected to a PJON media.
 * It is meant to be run in two instances, and will send incoming PJON messages through an ethernet tunnel to another instance 
 * of this sketch which will again deliver it to a PJON media on its side.
 * 
 * Arduino A - <PJON wired or wireless> - PJONEthernetTunnelB - <TCP/IP> - PJONEthernetTunnelA - <PJON wired or wireless> - Arduino B
 *  (ID 44)                                    (ID 45)                           (ID 44)                                     (ID 45)
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
uint8_t ids[] = { 45, 44 };
byte ip[][4] = { { 192, 168, 1, 10 }, { 192, 168, 1, 11 } };
byte mac[][6] = { { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }, { 0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xED } };  

// PJON settings
#define PJONPIN 7

//#define DEBUGPRINT  // Print debug information to serial port
//#define PJONLINK    // Use a PJON connection instead of EthernetLink for comparison

#ifdef DEBUGPRINT
int count_pjon = 0, count_ether = 0;
unsigned long lastprint = millis();
#endif

PJON<SoftwareBitBang> pjon(ids[this_device]);

#ifdef PJONLINK
#define PJONLINKPIN 6
PJON<SoftwareBitBang> pjon_link(ids[this_device]);
#else
EthernetLink ethernetLink(ids[this_device]);
#endif

void setup() {
#ifdef DEBUGPRINT
  Serial.begin(115200);
  Serial.print("Welcome to PJONEthernetTunneler, role ");
  Serial.println(this_device);
#endif
#ifdef PJONLINK
  pjon_link.set_pin(PJONLINKPIN);
  pjon_link.set_receiver(ethernet_receiver);
#else
  // Disable SD on Ethernet shield to avoid interference
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Ethernet.begin(mac[this_device], ip[this_device], gateway, subnet);
  delay(1000);
  ethernetLink.set_receiver(ethernet_receiver);
  ethernetLink.add_node(ids[1-this_device], ip[1-this_device]);
  ethernetLink.keep_connection(false);
  ethernetLink.start_listening();
#endif

  pjon.set_pin(PJONPIN);
  pjon.set_receiver(pjon_receiver);
}

unsigned long wp = 0, we = 0;

void loop() {
  unsigned long start = micros();
  pjon.receive(1000);
  pjon.update();
  wp += (micros()-start);

  start = micros();
#ifdef PJONLINK  
  pjon_link.receive(1000);
  pjon_link.update();
#else
  ethernetLink.receive(1000);
  ethernetLink.update();
#endif
  we += (micros()-start);

#ifdef DEBUGPRINT
  if (millis() - lastprint > 10000) {
    lastprint = millis();
    Serial.print("Forwarded "); Serial.print(count_pjon);
    Serial.print(" PJON packets and "); Serial.print(count_ether);
    Serial.print(" IP packets in 10s. WP="); Serial.print(wp);
    Serial.print(", WE="); Serial.println(we);
    count_pjon = count_ether = 0;
    wp = we = 0;
  }
#endif
}

void ethernet_receiver(uint8_t id, uint8_t *payload, uint8_t length) {
//  Serial.print("Forwarded Ethernet message from E_ID "); Serial.print(id);
//  Serial.print(" to P_ID device "); Serial.print(ids[1-this_device]);
//  print_payload(payload, length);
  pjon.send(ids[1-this_device], (char *)payload, length);
#ifdef DEBUGPRINT
  count_ether++;
#endif  
}

void pjon_receiver(uint8_t id, uint8_t *payload, uint8_t length) {
//  Serial.print("Forwarded PJON message from P_ID "); Serial.print(id);
//  Serial.print(" to E_ID device "); Serial.print(ids[1-this_device]);
//  print_payload(payload, length);
#ifdef PJONLINK
  int code = FAIL;
  int v = pjon_link.send(ids[1-this_device], (char *)payload, length);
  if (v != FAIL) code = ACK;
  pjon_link.update();
#else
  int code = ethernetLink.send_with_duration(ids[1-this_device], (char *)payload, length, 1000000);
#endif
#ifdef DEBUGPRINT
  if (code == ACK) count_pjon++;
#endif  
}

void print_payload(uint8_t *payload, uint8_t length) {
  char buf[length+1];
  memcpy(buf, payload, length);
  buf[length] = 0;
  Serial.print(", data:'"); Serial.print(buf); Serial.println("'.");
}

