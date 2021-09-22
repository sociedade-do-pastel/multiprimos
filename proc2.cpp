#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>

void SieveOfEratosthenes(int n, std::vector<int>* v)
{
	// inspirado no algoritmo de
	// https://www.geeksforgeeks.org/sieve-of-eratosthenes/
    std::unique_ptr<bool[]> prime = std::make_unique<bool[]>(n + 1);
	std::fill_n(prime.get(), n+1, true);

    for (int p{2}; p * p <= n; ++p) {
        if (prime[p]) {
            for (int i{p * p}; i <= n; i += p)
                prime[i] = false;
        }
    }

    for (int p{2}; p <= n; ++p){
        if (prime[p])
            v->push_back(p);
	}
}

int main()
{
    std::vector<int> v;
	SieveOfEratosthenes(2'000'000, &v);

    return 0;
}
