#include <iostream>
#include <mutex>
#include <cstdlib>
#include "pool.hpp"
#include <cmath>
#include <limits>
int main()
{
    threadpool pool;
    std::vector<std::shared_future<bool>> promises;
    for(unsigned i = 0; i <= 990000; i++)
    {
        auto f = pool.enqueue([](int i) 
        {
            if(i <= 1) return false;
            else if(i == 2) return true;
            else if(i % 2 == 0) return false;

            for(auto k = 3; k <= sqrt(i); k+=2)
            {
                if(i % k == 0) return false;
            }
            return true;
        }, i);
        promises.push_back(std::move(f));
    }
    for(unsigned i = 0; i < promises.size(); i++)
    {
        std::cout << i << (promises[i].get() ? " Is prime" : " Isn't prime") << std::endl;
    }
}