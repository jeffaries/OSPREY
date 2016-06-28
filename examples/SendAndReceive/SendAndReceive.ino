/*
 * SendAndReceive
 * This sketch is meant for an arduino with an Ethernet shield.
 * It is meant to be run in two instances, and will send messages to another instance
 * of this sketch which will also send messages to this instance.
 */

//#define DEBUGPRINT
#define LCD

#include <Arduino.h>
#include <EthernetLink.h>
#ifdef LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 20 chars and 4 line display
#endif

// Basic Ethernet settings
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Devices
uint8_t this_device = 1; // 0 or 1, chooses which Ethernet device this is
uint8_t ids[] = { 44, 45 };
byte ip[][4] = { { 192, 168, 1, 10 }, { 192, 168, 1, 11 } };
byte mac[][6] = { { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }, { 0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xED } };

// Status print variables
int count_receive = 0, count_send = 0;
unsigned long lastprint = millis();

EthernetLink ethernetLink(ids[this_device]);

void setup() {
  Serial.begin(115200);
  Serial.print("Welcome to SendAndReceive, role ");
  Serial.println(this_device);

  #ifdef LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0,1);
    lcd.printstr("SendAndReceive");
    lcd.setCursor(0,2);
    lcd.printstr("Role "); lcd.print(this_device);
  #endif

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
  ethernetLink.receive();
  ethernetLink.update();

  // Send a packet
  static bool started_send = false, completed_send = false;
  static unsigned long last_send = 0;
  if (micros() - last_send >= 1000 || (started_send && !completed_send)) {
    last_send = micros();
    started_send = true; completed_send = false;
    int result = ethernetLink.send(ids[1-this_device], "HELLO!", 7, 1000);
    if (result == ACK) { count_send++; started_send = false; completed_send = false; }
  }

  // Show statistics
  if (millis() - lastprint > 10000) {
    lastprint = millis();

    #ifdef DEBUGPRINT
        Serial.print("Received "); Serial.print(count_receive);
        Serial.print(", sent "); Serial.print(count_send);
        Serial.println(" IP packets in 10s.");
    #endif

    #ifdef LCD
      lcd.setCursor(0,0);
      lcd.printstr("Received : "); lcd.print(count_receive); lcd.printstr("  ");
      lcd.setCursor(0,1);
      lcd.printstr("Sent     : "); lcd.print(count_send); lcd.printstr("  ");
    #endif

    count_receive = count_send = 0;
  }
}

void ethernet_receiver(uint8_t id, uint8_t *payload, uint8_t length) {

  #ifdef DEBUGPRINT
  //  Serial.print("Forwarded Ethernet message from E_ID "); Serial.print(id);
  //  Serial.print(" to P_ID device "); Serial.print(ids[1-this_device]);
  //  print_payload(payload, length);
  #endif

  count_receive++;
}

void print_payload(uint8_t *payload, uint8_t length) {
  char buf[length+1];
  memcpy(buf, payload, length);
  buf[length] = 0;
  Serial.print(", data:'"); Serial.print(buf); Serial.println("'.");
}
