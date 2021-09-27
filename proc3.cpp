#include <future>
#include <iostream>
#include <thread>

bool isPrime(const int n)
{
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

std::uint64_t nthPrime(const int code, const int n, const int step)
{
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

std::uint64_t calculateKey(int code, int n)
{
    std::future<std::uint64_t> f1 = std::async(
        std::launch::async, [code, n]() { return nthPrime(code, n, 1); });
    std::future<std::uint64_t> f2 = std::async(
        std::launch::async, [code, n]() { return nthPrime(code, n, -1); });

    f1.wait();
	f2.wait();
	
    return (f1.get() * f2.get());
}

int main(int argc, char* argv[])
{
    std::cout << calculateKey(1000234, 10000) << "\n";

    return 0;
}
