#include <Arduino.h>
#include <PJON.h>
#include <EthernetLink.h>

// TODO:
// 1. Checksum
// 2. Retransmission if not ACKED
// 3. Support ENC28J60 based Ethernet shields

// Magic number to verify that we are aligned with telegram start and end
#define HEADER 0x18ABC427
#define FOOTER 0x9ABE8873
#define SUCCESS 0x101

#define DEBUGPRINT

void EthernetLink::init() {
  _server = NULL;
  _current_device = -1;
  _keep_connection = false;
  _local_id = 0;
  memset(_local_ip, 0, 4);
  _local_port = DEFAULT_PORT;
  _receiver = NULL;
  _error = NULL;

  for(uint8_t i = 0; i < MAX_REMOTE_NODES; i++) {
    _remote_id[i] = 0;
    memset(_remote_ip[i], 0, 4);
    _remote_port[i] = 0;
  }
}


int16_t EthernetLink::read_bytes(EthernetClient &client, byte *contents, int length) {
//  return client.read(contents, length);
  int16_t bytes_read = 0;
  uint32_t start_ms = millis();
  for (int i = 0; i < length && client.connected() && !(millis() - start_ms >= 10000); i++) {
    while (client.available() == 0 && client.connected() && !(millis() - start_ms >= 10000)) ; // Wait for next byte
    contents[i] = client.read();
    bytes_read++;
  }
  return bytes_read;
};


uint16_t EthernetLink::receive() {
  if(_server == NULL) start_listening(_local_port);
  /*
  #ifdef DEBUGPRINT
    static unsigned long first = millis();
    if (millis() - first >= 5000) {
      first = millis();
      Serial.println("Waiting for client to connect...");
    }
  #endif
  */
  EthernetClient client = _server->available();
  if(client) {
    #ifdef DEBUGPRINT
      Serial.println("A client connected!");
    #endif

    // Read encapsulation header (4 bytes magic number)
    boolean header_status = true;
    int32_t head = 0;
    int16_t bytes_read = read_bytes(client, (byte*) &head, 4);

    if(bytes_read != 4 || head != HEADER) header_status = false;

    #ifdef DEBUGPRINT
      Serial.print("Read header, status: "); Serial.println(header_status);
    #endif

    uint8_t sender_id = 0;
    // Read sender device id
    if (header_status) {
      bytes_read = read_bytes(client, (byte*) &sender_id, 1);
      if(bytes_read != 1) header_status = false;
    }

    int32_t content_length = 0;
    // Read length of contents (4 bytes)
    if(header_status) {
      bytes_read = read_bytes(client, (byte*) &content_length, 4);
      if(bytes_read != 4 || content_length <= 0) header_status = false;
    }

    byte buf[content_length];
    // Read contents
    if (header_status) {
      bytes_read = read_bytes(client, (byte*) &buf, content_length);
      if(bytes_read != content_length) header_status = false;
    }

    int32_t foot = 0;
    // Read footer (4 bytes magic number)
    if(header_status) {
      bytes_read = read_bytes(client, (byte*) &foot, 4);
      if(bytes_read != 4 || foot != FOOTER) header_status = false;
    }

    #ifdef DEBUGPRINT
      Serial.print("Status before sending ACK: "); Serial.println(header_status);
    #endif

    int32_t returncode = header_status ? ACK : NAK;
    int acklen = 0;
    // Write ACK
    if(client.connected()) acklen = client.write((byte*) &returncode, 4);
      client.flush();

    #ifdef DEBUGPRINT
      Serial.print("Sent "); Serial.print(header_status ? "ACK: " : "NAK: "); Serial.println(acklen);
    #endif

    // Call receiver callback function
    if(header_status) _receiver(sender_id, buf, content_length);

    // Close connection
    if(!_keep_connection || !client.connected()) client.stop();
    return header_status;
  }
  return false;
  // TODO: Call error callback at appropriate times with appropriate codes
};


uint16_t EthernetLink::receive(uint32_t duration_us) {
  uint32_t start = micros();
  int16_t result = FAIL;
  do {
    result = receive();
  } while(result != ACK && !(micros() - start >= duration_us));
  return result;
};


uint16_t EthernetLink::send(uint8_t id, char *packet, uint8_t length, uint32_t iming_us) {
  // Locate the node's IP address and port number
  int16_t remote_id_index = find_remote_node(id);

  #ifdef DEBUGPRINT
    Serial.print("Send to server remote_id_index="); Serial.println(remote_id_index);
  #endif

  if(remote_id_index < 0) return FAIL;

  EthernetClient _client;

  // Try to deliver the package
  if(!_client.connected()) _client.stop();

  if(!_keep_connection || (_current_device != id && _client.connected())) {
    #ifdef DEBUGPRINT
      if (_keep_connection && _current_device != -1) Serial.println("Switching connection to another server.");
    #endif
    while(_client.connected()) _client.stop();
  }

  uint32_t start = micros();
  int32_t result = FAIL;

  #ifdef DEBUGPRINT
    if (!_client.connected()) Serial.println("Trying to connect...");
  #endif

  do {
    if(
      (_keep_connection && _client && _client.connected()) ||
      _client.connect(_remote_ip[remote_id_index], _remote_port[remote_id_index])
    ) {

      #ifdef DEBUGPRINT
        Serial.println("Connected to server!");
      #endif

      _current_device = id;

      int16_t bytes_sent = 0, bytes_total = 0;
	    int32_t head = HEADER, foot = FOOTER, len = length;

      bytes_sent = _client.write((byte*) &head, 4);
	    bytes_sent = _client.write((byte*) &id, 1);
	    bytes_sent = _client.write((byte*) &len, 4);

      do {
        bytes_sent = _client.write(&packet[bytes_total], length-bytes_total);
        bytes_total += bytes_sent;
      } while(bytes_sent > 0 && bytes_total < length);

      if(bytes_total == length) result = SUCCESS;
      bytes_sent = _client.write((byte*) &foot, 4);
      _client.flush();
    }
  } while(result != SUCCESS && (timing_us == 0 || !(micros() - start >= timing_us)));

  #ifdef DEBUGPRINT
    Serial.print("Write status: "); Serial.println(result == SUCCESS);
  #endif

  // Read ACK
  if (result == SUCCESS) {
    result = FAIL;
    int32_t code = 0;
    int16_t bytes_read = read_bytes(_client, (byte*) &code, 4);
    if(bytes_read == 4 && (code == ACK || code == NAK)) result = code;
  }

  #ifdef DEBUGPRINT
    Serial.print("ACK status: "); Serial.println(result == ACK);
  #endif

  // Disconnect
  if(!_keep_connection) _client.stop();
  return result;  // FAIL, ACK or NAK
};


int16_t EthernetLink::find_remote_node(uint8_t id) {
  for(int8_t i = 0; i < MAX_REMOTE_NODES; i++) if(_remote_id[i] == id) return i;
  return -1;
};


int16_t EthernetLink::add_node(uint8_t remote_id, const uint8_t remote_ip[], uint16_t port_number) {
  // Find free slot
  int remote_id_index = find_remote_node(0);
  if(remote_id_index < 0) return remote_id_index; // All slots taken

  _remote_id[remote_id_index] = remote_id;
  memcpy(_remote_ip[remote_id_index], remote_ip, 4);
  _remote_port[remote_id_index] = port_number;
  return remote_id_index;
};


void EthernetLink::start_listening(uint16_t port_number) {
  if(_server != NULL) return; // Already started

  #ifdef DEBUGPRINT
    Serial.print("Started listening for connections on port "); Serial.println(port_number);
  #endif

  _server = new EthernetServer(port_number);
  _server->begin();
};


void EthernetLink::stop_listening() {
  if(_server) delete _server; _server = NULL;
};
