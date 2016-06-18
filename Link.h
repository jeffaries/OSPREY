#ifndef _LINK_H_
#define _LINK_H_

/*

  General transport Interface
  This is the basic Link Class.
  PJON and PJON_ASK are two examples of an OSPREY transport.
  If you want to write you own physical layer Transport library and support
  OSPREY, see the PJON Standard definition: https://github.com/gioblu/PJON/wiki
  Giovanni Blu Mitolo 2015 - gioscarab@gmail.com

 */


typedef void (* receiver)(uint8_t id, uint8_t *payload, uint8_t length);
typedef void (* error)(uint8_t code, uint8_t data);

class Link {
  public:
    virtual int receive() = 0;
    virtual int receive(unsigned long duration) = 0;

    virtual int send(uint8_t id, char *packet, uint8_t length, unsigned long timing = 0) = 0;

    virtual void    set_id(uint8_t id) = 0;
    virtual void    set_error(error e) = 0;
    virtual void    set_receiver(receiver r) = 0;

    virtual uint8_t device_id() = 0;
    virtual uint8_t acquire_id() = 0;

    virtual void update() = 0;
};

#endif