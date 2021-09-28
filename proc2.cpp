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
  return (key >= 1'000'000
	  && key >= sieve_of_erastosthenes (2 * n));
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

  /* check if the key sent is valid */
  if (check_key (key, n))
    {
      std::unique_ptr <sock_info> proc3_info {get_info ("20002")};

      /* it's gonna be faster if you init p3 FIRST */
      while (connect (proc3_info->descriptor,
		      proc3_info->result->ai_addr,
		      proc3_info->result->ai_addrlen) != -1);

      
      /* send in the string */
      send (proc3_info->descriptor, buffer, buffer_size, 0);

      /* get our final result into our buffer*/
      std::fill (buffer, buffer + buffer_size, '\0');
      recv (proc3_info->descriptor, buffer, buffer_size - 1, 0);

      /* send it back to p1 */
      send (p1_info.descriptor, buffer, buffer_size, 0);
    }
  else
    {
      send (p1_info.descriptor, "d", 2, 0);
    }

  return 0;
}
