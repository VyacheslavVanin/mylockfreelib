#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <stdint.h>
#include "sources/objectpool.hpp"

struct foo{
    enum{FOO_ARRAY_SIZE=5};
    int arr[FOO_ARRAY_SIZE];
    int cs;

    foo(int s=1) : cs(0)
    {
        for(size_t i = 0; i < FOO_ARRAY_SIZE; ++i)
            arr[i] = (s << i)*123;
        cs = getcs(*this);
    }

    static int getcs(const foo& f)
    {
        int ret = 0;
        int m = 1;
        for(size_t i=0; i < foo::FOO_ARRAY_SIZE; ++i) {
            ret += f.arr[i]*m;
            m = m*10 + 1; }
        return ret;
    }
};


blockAllocator<foo> ba(10);

void thread_func()
{
    int i=1;
    while(true) {
        auto f = ba.get(i++); 
        if( f == nullptr)
            std::cout << "FFFffff" <<std::endl;

        if(f->cs != foo::getcs(*f))
            std::cout << "Fuuuu" <<std::endl;
        ba.free(f);
    }
}


int main()
{
    std::thread t1 = std::thread(thread_func);
    std::thread t2 = std::thread(thread_func);
        t1.join();
        t2.join();
    std::cout << std::endl;
}
