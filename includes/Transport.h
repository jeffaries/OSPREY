
/*

  General transport Interface
  This is the basic Transport Class.
  PJON and PJON_ASK are two examples of an OSPREY transport.
  If you want to write you own physical layer Transport library and support
  OSPREY, see the PJON Standard definition: https://github.com/gioblu/PJON/wiki
  Giovanni Blu Mitolo 2015 - gioscarab@gmail.com

 */

class Transport {
  public:
    virtual ~Transport(int input_pin);
    virtual ~Transport(int input_pin, uint8_t id);

    virtual int receive(unsigned long duration);
    virtual int send(uint8_t id, char *packet, uint8_t length, unsigned long timing = 0);

    virtual void set_id(uint8_t id);
    virtual void set_error(error e);
    virtual void set_receiver(receiver r);

    virtual void update();
};
