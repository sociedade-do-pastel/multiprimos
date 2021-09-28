#include <future>
#include <iostream>
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

std::uint64_t calculateKey(int code, int n) {
  std::future<std::uint64_t> f1 = std::async(
      std::launch::async, [code, n]() { return nthPrime(code, n, 1); });
  std::future<std::uint64_t> f2 = std::async(
      std::launch::async, [code, n]() { return nthPrime(code, n, -1); });

  f1.wait();
  f2.wait();

  return (f1.get() * f2.get());
}

int main(int argc, char *argv[]) {

  /* copy-pasted buffer declaration and its size, no need to change it since
   we're gonna get the same string as p2 */
  int buffer_size{100};
  char buffer[buffer_size];
  std::fill(buffer, buffer + buffer_size, '\0');

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
    int recv_bytes{
        static_cast<int>(recv(p2_info.descriptor, buffer, buffer_size - 1, 0))};

    if (!recv_bytes)
      break;

    /* we're supposed to get key<whitespace>number */
    /* but regexp allows us to get any kind of number that is separated
       by any kind of separator, so that's nice (still can get garbage) */
    std::cmatch f_match;
    std::cmatch s_match;
    std::regex numb_regex{"([0-9]+)"};

    /* FUCKIN RAW */
    std::regex_search(buffer, f_match, numb_regex);
    std::uint64_t code{static_cast<uint64_t>(std::stoll(f_match[0]))};

    /* now we get our key (no error checking for now) */
    std::regex_search(buffer + f_match.length(), s_match, numb_regex);
    std::uint64_t n{static_cast<uint64_t>(std::stoll(s_match[0]))};

    std::string key{std::to_string(calculateKey(code, n))};
    int length{static_cast<int>(key.size() + 1)};

    if ((complete_send(p2_info.descriptor, key.c_str(), length)) == -1)
      std::cout << "error" << std::endl;
  }
  return 0;
}
