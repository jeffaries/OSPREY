#ifndef _ETHERNET_LINK_
	#define _ETHERNET_LINK_

	#include <Link.h>
	#include <Ethernet.h>
	#include <EthernetClient.h>
	#include <EthernetServer.h>

	#define MAX_REMOTE_NODES 10
	#define DEFAULT_PORT     9000

	class EthernetLink : public Link {
	private:
		receiver  _receiver;
		error 	  _error;

		// Local node
		uint8_t        _local_id;
		uint8_t        _local_ip[4];
		unsigned short _local_port;

		// Remote nodes
		uint8_t        _remote_id[MAX_REMOTE_NODES];
		uint8_t        _remote_ip[MAX_REMOTE_NODES][4];
		unsigned short _remote_port[MAX_REMOTE_NODES];

		EthernetServer *_server;
		EthernetClient _client;
		short _current_device;
		bool _keep_connection;

		void init();
		int16_t find_remote_node(uint8_t id);
		int16_t read_bytes(EthernetClient &client, byte *contents, int length);
	public:
		EthernetLink() { init(); }
		EthernetLink(uint8_t id) { init(); set_id(id); }

		int16_t add_node(uint8_t remote_id, const uint8_t remote_ip[], uint16_t port_number = DEFAULT_PORT);
		void start_listening(uint16_t port_number = DEFAULT_PORT);
		void stop_listening();

		// Whether to keep outgoing connection live until we need connect to another EthernetLink node
		void keep_connection(bool keep) { _keep_connection = keep; };

		/* Overridden functions below */

		uint16_t receive();
		uint16_t receive(uint32_t duration_us);

		uint16_t send(uint8_t id, char *packet, uint8_t length, unsigned long timing_us = 0);

		void set_id(uint8_t id) { _local_id = id; };
		void set_error(error e) { _error = e; };
		void set_receiver(receiver r) { _receiver = r; };

		uint8_t device_id() { return _local_id; };
		uint8_t acquire_id() { return 0; }; // Not supported yet

		void update() {};
	};

#endif
