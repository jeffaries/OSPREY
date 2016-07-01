#include <Arduino.h>
#include <PJON.h>
#include <EthernetLink.h>

// TODO:
// 1. Checksum
// 2. Retransmission if not ACKED
// 3. Support ENC28J60 based Ethernet shields
// 4. FIFO queue class common with PJON?

// Magic number to verify that we are aligned with telegram start and end
#define HEADER 0x18ABC427ul
#define FOOTER 0x9ABE8873ul

//#define DEBUGPRINT

void EthernetLink::init() {
  _server = NULL;
  _keep_connection = false;
  _local_id = 0;
  _current_device = -1;
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
  int16_t total_bytes_read = 0, bytes_read = 0;
  uint32_t start_ms = millis();
  // NOTE: The recv function here returns -1 if no data waiting, and 0 if socket closed!
  do {
    while (client.available() <= 0 && client.connected() && millis() - start_ms < 10000) ;
    bytes_read = client.read(&contents[total_bytes_read], length - total_bytes_read);
    if (bytes_read > 0) total_bytes_read += bytes_read;
  } while(bytes_read != 0 && total_bytes_read < length && millis() - start_ms < 10000);
  if (bytes_read == 0) client.stop(); // Lost connection  
  return total_bytes_read;
}

uint16_t EthernetLink::receive() {
  if(_server == NULL) start_listening(_local_port);
  // Accept new incoming connection if connection has been lost
  bool connected = _client_in.connected();
  if(!_keep_connection || !connected) {
    _client_in.stop();
    _client_in = _server->available();
    connected = _client_in;
    #ifdef DEBUGPRINT
      if(connected) Serial.println("A client connected!");
    #endif
  }

  int16_t return_value = FAIL;
  if(connected && _client_in.available() > 0) {

    #ifdef DEBUGPRINT
      Serial.println("Received data from client!");
    #endif

    // Read encapsulation header (4 bytes magic number)
    bool ok = true;
    uint32_t head = 0;
    int16_t bytes_read = 0;
    bytes_read = read_bytes(_client_in, (byte*) &head, 4);
    if(bytes_read != 4 || head != HEADER) { // Did not get header. Lost position in stream?
      do { // Try to resync if we lost position in the stream (throw avay all until HEADER found)
        head = head >> 8; // Make space for 8 bits to be read into the most significant byte
        bytes_read = read_bytes(_client_in, &((unsigned byte*) &head)[3], 1);
        if(bytes_read != 1) { ok = false; break; }
      } while(head != HEADER);
    }
    #ifdef DEBUGPRINT
      Serial.print("Read header, status: "); Serial.println(ok);
    #endif
        
    // Read sender device id (1 byte) and length of contents (4 bytes)
    uint8_t sender_id = 0;
    int16_t content_length = 0;
    if(ok) {
      byte buf[5];
      bytes_read = read_bytes(_client_in, buf, 5);
      if(bytes_read != 5) ok = false;
      else {
        memcpy(&sender_id, buf, 1);
        memcpy(&content_length, &buf[1], 4);
        if (content_length == 0) ok = 0;
      }
    }

    // Read contents and footer
    byte buf[content_length];
    if(ok) {
      bytes_read = read_bytes(_client_in, (byte*) buf, content_length);
      if(bytes_read != content_length) ok = false;
    }

    // Read footer (4 bytes magic number)
    if(ok) {
      uint32_t foot = 0;
      bytes_read = read_bytes(_client_in, (byte*) &foot, 4);
      if(bytes_read != 4 || foot != FOOTER) ok = false;
    }

    #ifdef DEBUGPRINT
      Serial.print("Status before sending ACK: "); Serial.println(ok);
    #endif

    // Write ACK
    int32_t returncode = ok ? ACK : NAK;
    int16_t acklen = 0;
    if(ok) {
      acklen = _client_in.write((byte*) &returncode, 4);
      if (acklen == 4) _client_in.flush();
    }
    return_value = returncode;
    
    #ifdef DEBUGPRINT
      Serial.print("Sent "); Serial.print(ok ? "ACK: " : "NAK: "); Serial.println(acklen);
    #endif

    // Call receiver callback function
    if(ok) _receiver(sender_id, buf, content_length);
  }

  // Close connection
  connected = _client_in.connected();
  if(!_keep_connection || !connected) {
    #ifdef DEBUGPRINT
      if (connected) Serial.println("Disconnecting incoming client."); 
    #endif
    _client_in.stop();
  }
  
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


uint16_t EthernetLink::send(uint8_t id, char *packet, uint8_t length, uint32_t timing_us) {
  // Locate the node's IP address and port number
  int16_t pos = find_remote_node(id);
  #ifdef DEBUGPRINT
    Serial.print("Send to server pos="); Serial.println(pos);
  #endif
  if(pos < 0) return FAIL;

  // Try to connect to server if not already connected
  bool connected = _client_out.connected();
  if (connected && _current_device != id) { // Connected, but to the wrong device
    #ifdef DEBUGPRINT
      //if(_keep_connection && _current_device != -1)
      Serial.println("Switching connection to another server.");
    #endif
    _client_out.stop();
    _current_device = -1;
    connected = false;
  }

  uint32_t start = millis();
  int16_t result = FAIL;

  if(!connected) {
    #ifdef DEBUGPRINT
      Serial.println("Trying to connect...");
    #endif
    connected = _client_out.connect(_remote_ip[pos], _remote_port[pos]);
    #ifdef DEBUGPRINT
      Serial.println(connected ? "Connected to server!" : "Failed in connecting to server!");
    #endif
    if(!connected) {
      _client_out.stop();
      _current_device = -1;
      return FAIL; // Server is unreachable or busy
    }
    _current_device = id;
  }

  // We are connected. Try to deliver the package
  uint32_t head = HEADER, foot = FOOTER, len = length;
  byte buf[9];
  memcpy(buf, &head, 4);
  memcpy(&buf[4], &id, 1);
  memcpy(&buf[5], &len, 4);
  bool ok = _client_out.write(buf, 9) == 9;
//  bool ok = _client_out.write((byte*) &head, 4) == 4;
//  if (ok) ok = _client_out.write((byte*) &id, 1) == 1;
//  if (ok) ok = _client_out.write((byte*) &len, 4) == 4;
  if (ok) ok = _client_out.write((byte*) packet, length) == length;
  if (ok) ok = _client_out.write((byte*) &foot, 4) == 4;
  if (ok) _client_out.flush();

  #ifdef DEBUGPRINT
    Serial.print("Write status: "); Serial.println(ok);
  #endif

  // If the other side is sending as well, we need to allow it to be read and ACKed,
  // otherwise we have a deadlock where both are waiting for ACK and will time out unsuccessfully.
  receive();

  // Read ACK
  if (ok) {
    uint32_t code = 0;
    ok = read_bytes(_client_out, (byte*) &code, 4) == 4;
    if (ok && (code == ACK || code == NAK)) result = code;
  }

  #ifdef DEBUGPRINT
    Serial.print("ACK status: "); Serial.println(result == ACK);
  #endif

  // Disconnect
  if (!ok || !_keep_connection) {
    _client_out.stop();
    #ifdef DEBUGPRINT
      Serial.print("Disconnected outgoing client. OK="); Serial.println(ok);
    #endif
  }
  
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
