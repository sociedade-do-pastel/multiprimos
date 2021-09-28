#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include <regex>

/* C libraries so we can use the sockets API at its fullest */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // for ``close ()''

/* "local" includes */
#include "sock.hpp"

/* General diagram */

/*
 +-------+  	+--------+ 
 |       |---->	|        | serv&cli     
 |   p1  |     	|   p2   |     
 |       | <----|        |     	    	  
 +-------+  	+--------+     	   	  
  cli  	    	   ^  |	       	   	  
		   |  |	       	   	  
		   |  v	       	   	  
		+--------+     	   	  
		|     	 |     	   	  
		|  p3    | serv	    	  
		|        |     	   
		+--------+     	   
*/

/*
 * p1 : will ``connect ()'' ``send ()'' in the string and wait for a response;
 * p2 : will ``bind ()'', ``listen ()'' for connections, ``recv ()'' that string
 * then ``connect () to p3, ``send ()'' a different string and ``recv ()'' the
 * result;
 * p3 : will ``bind ()'', ``listen ()'' and ``recv ()''.
 */

long long
sieve_of_erastosthenes (int n)
{
  // slightly modified algorithm which was originally found @
  // https://www.geeksforgeeks.org/sieve-of-eratosthenes/
  std::unique_ptr<bool[]> prime = std::make_unique<bool[]>(n + 1);
  std::fill_n(prime.get(), n+1, true);

  long long cnt{0};
  
  for (int p{2}; p * p <= n; ++p) {
    if (prime[p]) {
      for (int i{p * p}; i <= n; i += p)
	prime[i] = false;
    }
  }

  for (int p{2}; p <= n; ++p){
    if (prime[p])
      ++cnt;
  }
  return cnt;
}

bool
check_key (long long key, long long n)
{
  /* TODO: make this condition */
  return false;
}

int
main ()
{
  /* buffer for our string operations and its size in bytes */
  int buffer_size {100};
  char buffer[buffer_size];
  std::fill (buffer, buffer + buffer_size, '\0');

  
  // descriptor that we'll gonna use to communicate with p1
  std::unique_ptr <sock_info> proc2_info;
  
  try
    {
      proc2_info = get_info ("20001");
    }
  catch (int& error)
    {
      std::cerr << "Couldn't create a descriptor, exiting... " << std::endl;
      std::cerr << error << std::endl;
      return 1;
    }

  // if that socket is still hanging, reuse it
  int yes {1};
  if (setsockopt (proc2_info->descriptor, SOL_SOCKET,
		  SO_REUSEADDR, &yes, sizeof yes) == -1)
    std::cerr << "Couldn't reuse that socket\n";
  
  // don't judge me 
  if ((bind (proc2_info->descriptor,
	    proc2_info->result->ai_addr,
	    proc2_info->result->ai_addrlen) == -1)
      || listen (proc2_info->descriptor, 1) == -1)
    return 1;

  // accepting p1's connection (and everyone else)
  struct sockaddr_storage addr{}; 
  socklen_t addr_size {sizeof (addr)};

  sock_info p1_info{accept (proc2_info->descriptor,
			    (struct sockaddr *)&addr,
			    &addr_size), nullptr};
  
  /* now we can finally begin */
  int recv_bytes {static_cast<int>(recv (p1_info.descriptor,
					 buffer,
					 buffer_size - 1,
					 0))};

  /* we're supposed to get key<whitespace>number */
  /* but regexp allows us to get any kind of number that is separated
   by any kind of separator, so that's nice (still can get garbage) */
  std::cmatch f_match;
  std::cmatch s_match;
  std::regex numb_regex {"([0-9]+)"};

  /* FUCKIN RAW */
  std::regex_search (buffer, f_match, numb_regex);
  long long key {std::stoll (f_match[0])};

  /* now we get our key (no error checking for now) */
  std::regex_search (buffer + f_match.length (), s_match, numb_regex);
  long long n {std::stoll (s_match[0])};

  
  int length {buffer_size - 1};

  /* check if the key sent is valid */
  if (check_key (key, n))
    {
      
      std::unique_ptr <sock_info> proc3_info;
      try
	{
	  proc3_info = get_info ("20002");
	}
      catch (int& error)
	{
	  std::cerr << "Couldn't create a descriptor, exiting... " << std::endl;
	  std::cerr << error << std::endl;
	  return 1;
	}
      
      /* it'll only work if p3 is started first (no error checking) */
      if (connect (proc3_info->descriptor,
		   proc3_info->result->ai_addr,
		   proc3_info->result->ai_addrlen) == -1)
	  return 1;
      
      /* send in the string */
      complete_send (proc3_info->descriptor, buffer, length);
      
      /* get our final result into our buffer*/
      std::fill (buffer, buffer + buffer_size, '\0');
      int s = recv (proc3_info->descriptor, buffer, length, 0);

      /* send it back to p1 */
      complete_send (p1_info.descriptor, buffer, length);
    }
  else
    {
      std::string denied_message{"d"};
      length = denied_message.size () + 1;
      complete_send (p1_info.descriptor, denied_message.c_str (), length);
    }


  return 0;
}
