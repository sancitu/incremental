// Filename: netAddress.h
// Created by:  drose (08Feb00)
//
////////////////////////////////////////////////////////////////////

#ifndef NETADDRESS_H
#define NETADDRESS_H

#include <pandabase.h>

#include <prio.h>

////////////////////////////////////////////////////////////////////
// 	 Class : NetAddress
// Description : Represents a network address to which UDP packets may
//               be sent or to which a TCP socket may be bound.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA NetAddress {
public:
  NetAddress();
  NetAddress(const PRNetAddr &addr);

  bool set_any(int port);
  bool set_localhost(int port);
  bool set_host(const string &hostname, int port);

  void clear();

  int get_port() const;
  void set_port(int port);
  string get_ip() const;

  PRNetAddr *get_addr() const;

  void output(ostream &out) const;

private:
  PRNetAddr _addr;
};

INLINE ostream &operator << (ostream &out, const NetAddress &addr) {
  addr.output(out);
  return out;
}

#endif

