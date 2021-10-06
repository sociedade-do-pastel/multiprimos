#include <endian.h>
#include <future>
#include <iostream>
#include <netinet/in.h>
#include <regex>
#include <string>
#include <thread>

#include "sock.hpp"

bool isPrime(const int n) {
  // extracted from:
  // https://learnprogramo.com/prime-number-program-in-c-plus-plus/

  // Corner cases
  if (n <= 1)
    return false;
  if (n <= 3)
    return true;
  // This is checked so that we can skip
  // middle five numbers in below loop
  if (n % 2 == 0 || n % 3 == 0)
    return false;
  for (int i{5}; i * i <= n; i += 6) {
    if (n % i == 0 || n % (i + 2) == 0)
      return false;
  }
  return true;
}

std::uint64_t nthPrime(const int code, const int n, const int step) {
  int qt{0};
  int lastPrime;

  for (int i{code}; qt != n; i += step) {
    if (isPrime(i)) {
      ++qt;
      lastPrime = i;
    }
  }

  return lastPrime;
}

std::uint64_t calculateKey(long code, long n) {
  std::future<std::uint64_t> f1 = std::async(
      std::launch::async, [code, n]() { return nthPrime(code + 1, n, 1); });
  std::future<std::uint64_t> f2 = std::async(
      std::launch::async, [code, n]() { return nthPrime(code - 1, n, -1); });

  f1.wait();
  f2.wait();

  return (f1.get() * f2.get());
}

int main(int argc, char *argv[]) {
  std::uint32_t buffer_l{};
  std::uint64_t buffer_ll{};
  std::unique_ptr<sock_info> p3_info;

  try {
    p3_info = get_info("20002");
  } catch (int &error) {
    std::cerr << "Couldn't create a descriptor, exiting... " << std::endl;
    std::cerr << error << std::endl;
    return 1;
  }

  // if that socket is still hanging, reuse it
  int yes{1};
  if (setsockopt(p3_info->descriptor, SOL_SOCKET, SO_REUSEADDR, &yes,
                 sizeof yes) == -1)
    std::cerr << "Couldn't reuse that socket\n";

  // don't judge me
  if ((bind(p3_info->descriptor, p3_info->result->ai_addr,
            p3_info->result->ai_addrlen) == -1) ||
      listen(p3_info->descriptor, 1) == -1)
    return 1;

  // accepting p2's connection (and everyone else's)
  struct sockaddr_storage addr {};
  socklen_t addr_size{sizeof(addr)};

  sock_info p2_info{
      accept(p3_info->descriptor, (struct sockaddr *)&addr, &addr_size),
      nullptr};

  while (true) {
    long int code{};
    long int n{};

    /* get the code and convert it to host order */
    if (recv(p2_info.descriptor, &buffer_l, sizeof(buffer_l), 0) == 0)
      break;

    code = ntohl(buffer_l);
    /* now get n and convert it to host order */
    if (recv(p2_info.descriptor, &buffer_l, sizeof(buffer_l), 0) == 0)
      break;

    n = ntohl(buffer_l);

    /* generate our key and convert it to network long */
    uint64_t key{calculateKey(code, n)};
    buffer_ll = htobe64(key);

    /* send it */
    if ((send(p2_info.descriptor, &buffer_ll, sizeof buffer_ll, 0)) == -1)
      std::cout << "error" << std::endl;
  }
  return 0;
}
