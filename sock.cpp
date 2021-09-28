#include "sock.hpp"

#include <iostream>

std::unique_ptr <sock_info>
get_info (std::string port_number)
{
  /* kinda stupid, but here we are*/
  if (std::stoi (port_number) < 1025)
    throw 0; // 0 - passed in a reserved port number

  int sock_desc {};
  struct addrinfo hardcoded_info {0};
  struct addrinfo* result;

  /* IPV4, Stream socket (TCP) and use my hostname */
  hardcoded_info.ai_family = AF_UNSPEC;
  hardcoded_info.ai_socktype = SOCK_STREAM;
  hardcoded_info.ai_flags = AI_PASSIVE;


  int status {getaddrinfo(NULL, port_number.c_str (), &hardcoded_info, &result)};

  if (status != 0)
    throw 1; // couldn't get address info

  struct addrinfo* current{};
  for (current = result;
       current != nullptr;
       current = current->ai_next)
    {
      sock_desc = socket (current->ai_family,
			  current->ai_socktype,
			  current->ai_protocol);

      if (sock_desc != -1)
	break;
    }

  if (!current)
    throw 2; // couldn't get a socket descriptor

  return std::make_unique <sock_info> (sock_desc, current);
}

int complete_send (int descriptor, const char* buffer, int len)
{
  int actual_len {len};
  int bytes_left {actual_len};
  int sent {0};
  int total{0};

  for (; total < actual_len; total += sent, bytes_left -= sent)
    {
      if ((sent = send (descriptor, buffer + total, bytes_left, 0)) != -1)
	break;
    }

  actual_len = total;

  if (sent == -1)
    return -1;

  return actual_len;
}
