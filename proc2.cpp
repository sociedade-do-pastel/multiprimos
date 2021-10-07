#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <regex>

/* C libraries so we can use the sockets API at its fullest */
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // for ``close ()''

/* "local" includes */
#include "Timer.hpp"
#include "sock.hpp"

#define CACHESIZE 4'000'001

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

static long long primeCache[CACHESIZE];

void fill_cache()
{
    // slightly modified algorithm which was originally found @
    // https://www.geeksforgeeks.org/sieve-of-eratosthenes/
    bool prime[CACHESIZE + 1];
    std::fill_n(std::begin(prime), CACHESIZE, true);

    for (int p{2}; p * p <= CACHESIZE; ++p) {
        if (prime[p]) {
            for (int i{p * p}; i <= CACHESIZE; i += p)
                prime[i] = false;
        }
    }

    long long cnt{0};

    for (int p{2}; p <= CACHESIZE; ++p) {
        if (prime[p])
            ++cnt;
        primeCache[p] = cnt;
    }
}

std::unique_ptr<bool[]> sieve_of_erastosthenes(long long key)
{
    // slightly modified algorithm which was originally found @
    // https://www.geeksforgeeks.org/sieve-of-eratosthenes/
    std::unique_ptr<bool[]> prime = std::make_unique<bool[]>(key + 1);
    std::fill_n(prime.get(), key + 1, true);

    for (int p{2}; p * p <= key; ++p) {
        if (prime[p]) {
            for (int i{p * p}; i <= key; i += p)
                prime[i] = false;
        }
    }

    return prime;
}

bool check_key(long long key, long long n)
{
    if (key <= 1'000'000)
        return false;

    long long cnt{0};

    if (key <= CACHESIZE)
        cnt = primeCache[key];
    else {
        std::unique_ptr<bool[]> prime = sieve_of_erastosthenes(key);

        for (int p{2}; p <= key; ++p) {
            if (prime[p])
                ++cnt;
        }
    }

    if (cnt < 2 * n)
        return false;

    return true;
}

int main()
{
    /* buffer for our numeric operations */
    uint32_t buffer_l{};
    uint64_t buffer_ll{};

    // descriptor that we'll gonna use to communicate with p1
    std::unique_ptr<sock_info> proc2_info;

	{
		Timer::Timer t;
		fill_cache();
	}
	
    try {
        proc2_info = get_info("20001");
    }
    catch (int& error) {
        std::cerr << "Couldn't create a descriptor, exiting... " << std::endl;
        std::cerr << error << std::endl;
        return 1;
    }

    /* P3 */
    std::unique_ptr<sock_info> proc3_info;

    try {
        proc3_info = get_info("20002");
    }
    catch (int& error) {
        std::cerr << "Couldn't create a descriptor, exiting... " << std::endl;
        std::cerr << error << std::endl;
        return 1;
    }

    /* it'll only work if p3 is started first (no error checking) */
    if (connect(proc3_info->descriptor, proc3_info->result->ai_addr,
                proc3_info->result->ai_addrlen)
        == -1)
        return 1;

    // if that socket is still hanging, reuse it
    int yes{1};
    if (setsockopt(proc2_info->descriptor, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof yes)
        == -1)
        std::cerr << "Couldn't reuse that socket\n";

    // don't judge me
    if ((bind(proc2_info->descriptor, proc2_info->result->ai_addr,
              proc2_info->result->ai_addrlen)
         == -1)
        || listen(proc2_info->descriptor, 1) == -1)
        return 1;

    // accepting p1's connection (and everyone else)
    struct sockaddr_storage addr
    {
    };
    socklen_t addr_size{sizeof(addr)};

    sock_info p1_info{
        accept(proc2_info->descriptor, (struct sockaddr*)&addr, &addr_size),
        nullptr};

    while (true) {
        /* now we can finally begin */
        long int code{};
        long int n{};

        /* get our code and assign it from the buffer*/
        if (recv(p1_info.descriptor, &buffer_l, sizeof buffer_l, 0) == 0)
            break;

        code = ntohl(buffer_l);

        /* get our n */
        if (recv(p1_info.descriptor, &buffer_l, sizeof buffer_l, 0) == 0)
            break;

        n = ntohl(buffer_l);

        /* check if the key sent is valid */
        if (check_key(code, n)) {
            /* first we send in the code */
            buffer_l = htonl(code);
            send(proc3_info->descriptor, &buffer_l, sizeof buffer_l, 0);

            /* then, our n */
            buffer_l = htonl(n);
            send(proc3_info->descriptor, &buffer_l, sizeof buffer_l, 0);

            /* get our final result into our buffer*/
            if (recv(proc3_info->descriptor, &buffer_ll, sizeof buffer_ll, 0)
                == 0)
                break;

            /* send it back to p1 */
            send(p1_info.descriptor, &buffer_ll, sizeof buffer_ll, 0);
        }
        else {
            buffer_ll = 0;
            send(p1_info.descriptor, &buffer_ll, sizeof(buffer_ll), 0);
        }
    }

    return 0;
}
