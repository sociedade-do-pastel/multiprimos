#pragma once
#include <string>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // for ``close ()''

class sock_info {
public:
  int descriptor{0};
  struct addrinfo* result;

  sock_info (int sock, struct addrinfo* res) : descriptor{sock}, result{res}
  {}
  
  ~sock_info ()
  {
    freeaddrinfo (result);
    close (descriptor);
  }
};

std::unique_ptr <sock_info> get_info (std::string port_number);

int complete_send (int descriptor, const char* buffer, int length);
