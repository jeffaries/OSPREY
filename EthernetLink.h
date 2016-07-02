#ifndef _ETHERNET_LINK_
  #define _ETHERNET_LINK_

  #include <Link.h>
  #include <Ethernet.h>

  // Constants
  #define ACK           6
  #define NAK           21

  // Internal constants
  #define FAIL          0x100

  #define MAX_REMOTE_NODES 10
  #define DEFAULT_PORT     9000

  class EthernetLink : public Link {
  private:
    receiver _receiver;
    error    _error;

    // Local node
    uint8_t  _local_id;
    uint8_t  _local_ip[4];
    uint16_t _local_port;

    // Remote nodes
    uint8_t  _remote_id[MAX_REMOTE_NODES];
    uint8_t  _remote_ip[MAX_REMOTE_NODES][4];
    uint16_t _remote_port[MAX_REMOTE_NODES];

    EthernetServer *_server;
    EthernetClient _client_out; // Created as an outgoing connection
    EthernetClient _client_in;  // Accepted incoming connection
  	int16_t _current_device;    // The id of the remove device/node that we have connected to
  	bool _keep_connection,      // Keep sockets permanently open instead of reconnecting for each transfer
         _single_socket;        // Do bidirectional transfer on a single socket

  	void init();
  	int16_t find_remote_node(uint8_t id);
  	int16_t read_bytes(EthernetClient &client, byte *contents, uint16_t length);
    uint16_t receive(EthernetClient &client);
    bool connect(uint8_t id);
    void stop(EthernetClient &client) { client.stop(); }
    bool accept();
    void disconnect_out_if_needed(int16_t result);
    bool disconnect_in_if_needed();
    uint16_t send(EthernetClient &client, uint8_t id, char *packet, uint16_t length);
    uint16_t single_socket_transfer(EthernetClient &client, int16_t id, bool master, char *contents, uint16_t length);
    bool read_until_header(EthernetClient &client, uint32_t header);
  public:
  	EthernetLink() { init(); };
  	EthernetLink(uint8_t id) { init(); set_id(id); };

  	int16_t add_node(uint8_t remote_id, const uint8_t remote_ip[], uint16_t port_number = DEFAULT_PORT);
  	void start_listening(uint16_t port_number = DEFAULT_PORT); // Do not call for single_socket master

  	// Whether to keep outgoing connection live until we need connect to another EthernetLink node
  	void keep_connection(bool keep) { _keep_connection = keep; };

    // Whether to do bidirectional data transfer on a single socket or use one socket for each direction.
    // Single socket transfer is slower but more firewall-friendly. Client connects to server, and packets are 
    // exchanged in both directions on the same socket. If using this, _only one_ of the sides should call
    // start_listening, and that side will be slave with the other master (establishing connections)
    void single_socket(bool single_socket) { _single_socket = single_socket; }
    
    // Keep trying to send for a maximum duration
    int16_t send_with_duration(uint8_t id, char *packet, uint8_t length, unsigned long duration_us);

  	//**************** Overridden functions below *******************

    uint16_t receive();
    uint16_t receive(unsigned long duration_us);

  	uint16_t send(uint8_t id, char *packet, uint8_t length, unsigned long timing_us = 0);

    void set_id(uint8_t id) { _local_id = id; };
    void set_error(error e) { _error = e; };
    void set_receiver(receiver r) { _receiver = r; };

    uint8_t device_id() { return _local_id; };
    uint8_t acquire_id() { return 0; }; // Not supported yet

    void update() {};
  };

#endif
