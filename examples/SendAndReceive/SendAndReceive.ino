/*
 * SendAndReceive
 * This sketch is meant for an arduino with an Ethernet shield.
 * It is meant to be run in two instances, and will send messages to another instance 
 * of this sketch which will also send messages to this instance.
 */

#include <Arduino.h>
#include <EthernetLink.h>

// Basic Ethernet settings
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Devices
uint8_t this_device = 1; // 0 or 1, chooses which Ethernet device this is
uint8_t ids[] = { 44, 45 };
byte ip[][4] = { { 192, 168, 1, 10 }, { 192, 168, 1, 11 } };
byte mac[][6] = { { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }, { 0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xED } };  

#define DEBUGPRINT

#ifdef DEBUGPRINT
int count_receive = 0, count_send = 0;
unsigned long wr = 0, ws = 0;
unsigned long lastprint = millis();
#endif

EthernetLink ethernetLink(ids[this_device]);

void setup() {
  Serial.begin(115200);
  Serial.print("Welcome to SendAndReceive, role ");
  Serial.println(this_device);

  // Disable SD on Ethernet shield to avoid interference
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Ethernet.begin(mac[this_device], ip[this_device], gateway, subnet);
  delay(1000);
  ethernetLink.set_receiver(ethernet_receiver);
  ethernetLink.add_node(ids[1-this_device], ip[1-this_device]);
  ethernetLink.keep_connection(true);
  ethernetLink.start_listening();
}

void loop() {
  unsigned long start = micros();
  ethernetLink.receive();
  wr += (micros()-start);
  ethernetLink.update();

  // Send a packet
  static bool started_send = false, completed_send = false;
  static unsigned long last_send = 0;
  if (micros() - last_send >= 1000 || (started_send && !completed_send)) {
    last_send = micros();
    start = micros();
    started_send = true; completed_send = false;
    int result = ethernetLink.send(ids[1-this_device], "HELLO!", 7, 1000);
    ws += (micros()-start);
    if (result == ACK) { count_send++; started_send = false; completed_send = false; }
  }

#ifdef DEBUGPRINT
  // Show statistics
  if (millis() - lastprint > 10000) {
    lastprint = millis();
    Serial.print("Received "); Serial.print(count_receive);
    Serial.print(", sent "); Serial.print(count_send);
    Serial.print(" IP packets in 10s. WR="); Serial.print(wr);
    Serial.print(", WS="); Serial.println(ws);
    count_receive = count_send = 0;
    wr = ws = 0;
  }
#endif  
}

void ethernet_receiver(uint8_t id, uint8_t *payload, uint8_t length) {
#ifdef DEBUGPRINT  
//  Serial.print("Forwarded Ethernet message from E_ID "); Serial.print(id);
//  Serial.print(" to P_ID device "); Serial.print(ids[1-this_device]);
//  print_payload(payload, length);
  count_receive++;
#endif
}

void print_payload(uint8_t *payload, uint8_t length) {
  char buf[length+1];
  memcpy(buf, payload, length);
  buf[length] = 0;
  Serial.print(", data:'"); Serial.print(buf); Serial.println("'.");
}

