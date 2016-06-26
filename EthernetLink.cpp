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

//#define DEBUGPRINT

void EthernetLink::init() { 
  _server = NULL;
  _currentDevice = -1;
  _keep_connection = false;
  _local_id = 0;
  memset(_local_ip, 0, 4);
  _local_port = DEFAULT_PORT;
  _receiver = NULL;
  _error = NULL; 
  for (int i=0; i<MAX_REMOTE_NODES; i++) {
    _remote_id[i] = 0;
    memset(_remote_ip[i], 0, 4);
    _remote_port[i] = 0;
  }
}

int EthernetLink::read_bytes(EthernetClient &client, byte *contents, int length) {
//  return client.read(contents, length);
  int bytes_read = 0;  
  unsigned long start_ms = millis(); 
  for (int i=0; i<length && client.connected() && !(millis() - start_ms >= 10000); i++) {
  while (client.available() == 0 && client.connected() && !(millis() - start_ms >= 10000)) ; // Wait for next byte    
    contents[i] = client.read();
    bytes_read++;
  }
  return bytes_read;
}

int EthernetLink::receive() {
  if (_server == NULL) start_listening(_local_port);  
/*  
#ifdef DEBUGPRINT
  static unsigned long first = millis();
  if (millis() - first >= 5000) {
    first = millis();  
    Serial.println("Waiting for client to connect..."); 
  }
#endif
*/
  // Accept new incoming connection if connection has been lost
  if (!_keep_connection || !_client_in.connected()) {
    _client_in.stop();
    _client_in = _server->available();
#ifdef DEBUGPRINT
    if (_client_in == true) Serial.println("A client connected!"); 
#endif
  }
  int return_value = FAIL;
  if (_client_in.connected() && _client_in.available()) {
#ifdef DEBUGPRINT
    Serial.println("Received data from client!"); 
#endif    
    // Read encapsulation header (4 bytes magic number)
    bool ok = true;  
    long head = 0;
    int bytes_read = read_bytes(_client_in, (byte*) &head, 4);
    if (bytes_read != 4 || head != HEADER) ok = false;
#ifdef DEBUGPRINT
    Serial.print("Read header, status: "); Serial.println(ok); 
#endif
  
    // Read sender device id
    uint8_t sender_id = 0;
    if (ok) {
      bytes_read = read_bytes(_client_in, (byte*) &sender_id, 1);
      if (bytes_read != 1) ok = false;
    }
  
    // Read length of contents (4 bytes)
    long content_length = 0;
    if (ok) {
      bytes_read = read_bytes(_client_in, (byte*) &content_length, 4);
      if (bytes_read != 4 || content_length <= 0) ok = false;
    }
  
    // Read contents
    byte buf[content_length];
    if (ok) {
      bytes_read = read_bytes(_client_in, (byte*) &buf, content_length);
      if (bytes_read != content_length) ok = false;
    }
    
    // Read footer (4 bytes magic number)
    long foot = 0;
    if (ok) {    
      bytes_read = read_bytes(_client_in, (byte*) &foot, 4);
      if (bytes_read != 4 || foot != FOOTER) ok = false;
    }
#ifdef DEBUGPRINT
    Serial.print("Status before sending ACK: "); Serial.println(ok); 
#endif

    // Write ACK
    long returncode = ok ? ACK : NAK;
    int acklen = 0;
    if (_client_in.connected()) {
      acklen = _client_in.write((byte*) &returncode, 4);
      _client_in.flush();
    }
#ifdef DEBUGPRINT
    Serial.print("Sent "); Serial.print(ok ? "ACK: " : "NAK: "); Serial.println(acklen); 
#endif
  
    // Call receiver callback function
    if (ok) _receiver(sender_id, buf, content_length);
  
    // Close connection
    if (!_keep_connection || !_client_in.connected()) _client_in.stop();
  
    return_value = returncode;
  }
  
  // Close connection
  if (!_keep_connection || !_client_in.connected()) _client_in.stop();

  return return_value;
}

// TODO: Call error callback at appropriate times with appropriate codes


int EthernetLink::receive(unsigned long duration_us) {
  unsigned long start = micros();
  int result = FAIL;
  do {
    result = receive();
  } while (result != ACK && micros() - start <= duration_us);
  return result;
}

int EthernetLink::send(uint8_t id, char *packet, uint8_t length, unsigned long timing_us) {
  // Locate the node's IP address and port number
  int pos = find_remote_node(id);
#ifdef DEBUGPRINT
  Serial.print("Send to server pos="); Serial.println(pos); 
#endif
  if (pos < 0) return FAIL;
  
  // Try to connect to server if not already connected
  if (_keep_connection && !_client_out.connected()) _client_out.stop();
  if (!_keep_connection  || (_currentDevice != id && _client_out.connected())) {
#ifdef DEBUGPRINT
    if (_keep_connection && _currentDevice != -1) Serial.println("Switching connection to another server.");
#endif
    _client_out.stop();
  }
  unsigned long start = millis();
  int result = FAIL;
#ifdef DEBUGPRINT
  if (!_client_out.connected()) Serial.println("Trying to connect...");
#endif
//  const int SEND_TIMEOUT = 10000;  
  bool connected = _keep_connection && _client_out.connected();  
  if (!connected) { 
    connected = _client_out.connect(_remote_ip[pos], _remote_port[pos]); 
#ifdef DEBUGPRINT
    Serial.println(connected ? "Connected to server!" : "Failed in connecting to server!");
#endif
  }
  if (!connected) {
    _client_out.stop();
    return FAIL; // Server is unreachable or busy
  }
  
  // Try to deliver the package
  _currentDevice = id;
  int bytes_sent = 0, bytes_total = 0;
  long head = HEADER, foot = FOOTER, len = length;
  bytes_sent = _client_out.write((byte*) &head, 4);
  bytes_sent = _client_out.write((byte*) &id, 1);
  bytes_sent = _client_out.write((byte*) &len, 4);
  do {  
    bytes_sent = _client_out.write(&packet[bytes_total], length-bytes_total);    
    bytes_total += bytes_sent;
  } while (bytes_sent > 0 && bytes_total < length);
  if (bytes_total == length) result = SUCCESS;
  bytes_sent = _client_out.write((byte*) &foot, 4);
  if (result == SUCCESS) _client_out.flush();  
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
  if (!_keep_connection) _client_out.stop();
  
  return result;  // FAIL, ACK or NAK
}

int EthernetLink::send_with_duration(uint8_t id, char *packet, uint8_t length, unsigned long duration_us) {
  unsigned long start = micros();
  int result = FAIL;
  do {
    result = send(id, packet, length);
  } while (result == FAIL && micros() - start <= duration_us);
  return result;
}

int EthernetLink::find_remote_node(uint8_t id) {
  for (int i=0; i<MAX_REMOTE_NODES; i++) if (_remote_id[i] == id) return i;
  return -1;
}

int EthernetLink::add_node(uint8_t remote_id, const uint8_t remote_ip[], uint16_t port_number) {
  // Find free slot
  int pos = find_remote_node(0);
  if (pos < 0) return pos; // All slots taken

  _remote_id[pos] = remote_id;
  memcpy(_remote_ip[pos], remote_ip, 4);
  _remote_port[pos] = port_number;
  return pos;
}

void EthernetLink::start_listening(uint16_t port_number) {
  if (_server != NULL) return; // Already started  
#ifdef DEBUGPRINT
  Serial.print("Started listening for connections on port "); Serial.println(port_number); 
#endif    
  _server = new EthernetServer(port_number);  
  _server->begin();
}

void EthernetLink::stop_listening() {
  if (_server) delete _server; _server = NULL;  
}
