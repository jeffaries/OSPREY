#include <Arduino.h>
#include <PJON.h>
#include <EthernetLink.h>

// TODO:
// 0. Resync stream if lost position, search for HEADER
// 1. Checksum
// 2. Retransmission if not ACKED
// 3. Support ENC28J60 based Ethernet shields
// 4. FIFI queue class common with PJON?

// PJON max message size, accept setting it from outside (#ifndef SIZE #define SIZE 50 #endif)

// Magic number to verify that we are aligned with telegram start and end
#define HEADER 0x18ABC427
#define FOOTER 0x9ABE8873
#define SUCCESS 0x101

//#define DEBUGPRINT

void EthernetLink::init() {
  _server = NULL;
  _current_device = -1;
  _keep_connection = false;
  _local_id = 0;
  memset(_local_ip, 0, 4);
  _local_port = DEFAULT_PORT;
  _receiver = NULL;
  _error = NULL;

  for (uint16_t i = 0; i < MAX_REMOTE_NODES; i++) {
    _remote_id[i] = 0;
    memset(_remote_ip[i], 0, 4);
    _remote_port[i] = 0;
  }
}


int16_t EthernetLink::read_bytes(EthernetClient &client, byte *contents, int length) {
  int16_t bytes_read = 0;
  uint32_t start_ms = millis();

  for(uint16_t i = 0; i < length && client.connected() && !(millis() - start_ms >= 10000); i++) {
    while(client.available() == 0 && client.connected() && !(millis() - start_ms >= 10000)) ; // Wait for next byte
    contents[i] = client.read();
    bytes_read++;
  }
  return bytes_read;
}

int16_t EthernetLink::receive() {
  if(_server == NULL) start_listening(_local_port);
  // Accept new incoming connection if connection has been lost
  if(!_keep_connection || !_client_in.connected()) {
    _client_in.stop();
    _client_in = _server->available();

    #ifdef DEBUGPRINT
      if(_client_in == true) Serial.println("A client connected!");
    #endif
  }

  int16_t return_value = FAIL;
  if(_client_in.connected() && _client_in.available()) {

    #ifdef DEBUGPRINT
      Serial.println("Received data from client!");
    #endif

    // Read encapsulation header (4 bytes magic number)
    bool ok = true;
    int32_t head = 0;
    int16_t bytes_read = 0;
    do { // Try to resync if we lost position in the stream (throw avay all until HEADER found)
      bytes_read = read_bytes(_client_in, (byte*) &head, 4);
      if(bytes_read != 4) { ok = false; break; }
    } while(head != HEADER);

    #ifdef DEBUGPRINT
      Serial.print("Read header, status: "); Serial.println(ok);
    #endif

    // Read sender device id

    uint8_t sender_id = 0;
    if(ok) {
      bytes_read = read_bytes(_client_in, (byte*) &sender_id, 1);
      if(bytes_read != 1) ok = false;
    }

    // Read length of contents (4 bytes)
    int16_t content_length = 0;
    if(ok) {
      bytes_read = read_bytes(_client_in, (byte*) &content_length, 4);
      if(bytes_read != 4 || content_length <= 0) ok = false;
    }

    // Read contents
    byte buf[content_length];
    if(ok) {
      bytes_read = read_bytes(_client_in, (byte*) &buf, content_length);
      if(bytes_read != content_length) ok = false;
    }

    // Read footer (4 bytes magic number)
    int32_t foot = 0;
    if(ok) {
      bytes_read = read_bytes(_client_in, (byte*) &foot, 4);
      if(bytes_read != 4 || foot != FOOTER) ok = false;
    }

    #ifdef DEBUGPRINT
      Serial.print("Status before sending ACK: "); Serial.println(ok);
    #endif

    // Write ACK
    int32_t returncode = ok ? ACK : NAK;
    int16_t acklen = 0;

    if(_client_in.connected()) {
      acklen = _client_in.write((byte*) &returncode, 4);
      _client_in.flush();
    }

    #ifdef DEBUGPRINT
      Serial.print("Sent "); Serial.print(ok ? "ACK: " : "NAK: "); Serial.println(acklen);
    #endif

    // Call receiver callback function
    if(ok) _receiver(sender_id, buf, content_length);

    // Close connection
    if(!_keep_connection || !_client_in.connected()) _client_in.stop();

    return_value = returncode;
  }

  // Close connection
  if(!_keep_connection || !_client_in.connected()) _client_in.stop();

  return return_value;
};

// TODO: Call error callback at appropriate times with appropriate codes


uint16_t EthernetLink::receive(uint32_t duration_us) {
  uint32_t start = micros();
  int16_t result = FAIL;
  do {
    result = receive();
  } while(result != ACK && micros() - start <= duration_us);
  return result;
};


uint16_t EthernetLink::send(uint8_t id, char *packet, uint8_t length, uint32_t iming_us) {
  // Locate the node's IP address and port number
  int16_t pos = find_remote_node(id);

  #ifdef DEBUGPRINT
    Serial.print("Send to server pos="); Serial.println(pos);
  #endif

  if(pos < 0) return FAIL;

  // Try to connect to server if not already connected
  if(_keep_connection && !_client_out.connected()) _client_out.stop();
  if(!_keep_connection  || (_currentDevice != id && _client_out.connected())) {
    #ifdef DEBUGPRINT
      if(_keep_connection && _currentDevice != -1) Serial.println("Switching connection to another server.");
    #endif
    _client_out.stop();
  }

  uint32_t start = millis();
  int16_t result = FAIL;

  #ifdef DEBUGPRINT
    if(!_client_out.connected()) Serial.println("Trying to connect...");
  #endif

  bool connected = _keep_connection && _client_out.connected();
  if(!connected) {
    connected = _client_out.connect(_remote_ip[pos], _remote_port[pos]);
    #ifdef DEBUGPRINT
      Serial.println(connected ? "Connected to server!" : "Failed in connecting to server!");
    #endif
  }
  if(!connected) {
    _client_out.stop();
    return FAIL; // Server is unreachable or busy
  }

  // Try to deliver the package
  _currentDevice = id;
  int16_t bytes_sent = 0, bytes_total = 0;
  int32_t head = HEADER, foot = FOOTER, len = length;
  bytes_sent = _client_out.write((byte*) &head, 4);
  bytes_sent = _client_out.write((byte*) &id, 1);
  bytes_sent = _client_out.write((byte*) &len, 4);
  do {
    bytes_sent = _client_out.write(&packet[bytes_total], length-bytes_total);
    bytes_total += bytes_sent;
  } while(bytes_sent > 0 && bytes_total < length);

  if(bytes_total == length) result = SUCCESS;
  bytes_sent = _client_out.write((byte*) &foot, 4);
  if(result == SUCCESS) _client_out.flush();

  #ifdef DEBUGPRINT
    Serial.print("Write status: "); Serial.println(result == SUCCESS);
  #endif

  // If the other side is sending as well, we need to allow it to be read and ACKed,
  // otherwise we have a deadlock where both are waiting for ACK and will time out unsuccessfully.
  receive();

  // Read ACK
  if (result == SUCCESS) {
    result = FAIL;
    long code = 0;
    int bytes_read = read_bytes(_client_out, (byte*) &code, 4);
    if (bytes_read == 4 && (code == ACK || code == NAK)) result = code;
  }

  #ifdef DEBUGPRINT
    Serial.print("ACK status: "); Serial.println(result == ACK);
  #endif

  // Disconnect
  if(!_keep_connection) _client_out.stop();
  return result;  // FAIL, ACK or NAK
};


int16_t EthernetLink::send_with_duration(uint8_t id, char *packet, uint8_t length, unsigned long duration_us) {
  uint32_t start = micros();
  int16_t result = FAIL;
  do {
    result = send(id, packet, length);
  } while(result == FAIL && micros() - start <= duration_us);
  return result;
};

int16_t EthernetLink::find_remote_node(uint8_t id) {
  for(int i = 0; i < MAX_REMOTE_NODES; i++) if(_remote_id[i] == id) return i;
  return -1;
};


int16_t EthernetLink::add_node(uint8_t remote_id, const uint8_t remote_ip[], uint16_t port_number) {
  // Find free slot
  int16_t remote_id_index = find_remote_node(0);
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
