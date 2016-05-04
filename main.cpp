#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <stdint.h>
#include <initializer_list>
#include "sources/objectpool.hpp"
#include "sources/ringbuffer1M.hpp"

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


void test_ba()
{
    using namespace std;
    ObjectPool<foo> ba(10);
    vector<thread> threads;
   
    auto thread_func = [&]() -> void{
        int i=1;
        while(true) {
            auto f = ba.get(i++); 
            if( f == nullptr)
                std::cout << "FFFffff" <<std::endl;

            if(f->cs != foo::getcs(*f))
                std::cout << "Fuuuu" <<std::endl;
            ba.free(f);
        }
    };

    for(size_t i = 0; i < 2; ++i)
        threads.push_back(thread(thread_func));

    for(auto& t:threads)
        t.join();
    std::cout << std::endl;
}



void test_rb()
{
    using namespace std;
    constexpr uint32_t numReaders = 1;

    RingBuffer1M rb(numReaders, 10, sizeof(foo), 1);

    vector<thread> readerThreads;

    auto readerThread = [&](){
        while(true){
            foo* p = (foo*)rb.pop_ext();
            if(p){
                if(p->cs != foo::getcs(*p))
                    std::cout << "Fuuu" <<std::endl;
                rb.release(p);
            }
        }
    };

    auto writerThread = [&](){
        int i = 1;
        while(true){
            foo* p = (foo*)rb.getBlank_nothrow();
            if(p){
                new(p)foo(i++);
                rb.pushBlank(p);
            }
            else{
                std::cout << "no free blocks" <<std::endl;
            }
        }
    };

    for(size_t i =0; i < numReaders; ++i)
        readerThreads.push_back(thread(readerThread));
    thread wt(writerThread);

    for(auto& t: readerThreads)
        t.join();
    wt.join();
}


static void push_rb(RingBuffer1M& rb)
{
    auto p = rb.getBlank();
    auto pp = rb.pushBlank_ext(p);
    if(pp)
        rb.release(pp);
}

static void pop_rb(RingBuffer1M& rb)
{
    auto p = rb.pop_ext();
    if(p)
        rb.release(p);
    else
        std::cout << "container empty" <<std::endl;
}

template<typename T>
static void do_n_times(const T& f, uint32_t n = 1)
{
    for(uint32_t i =0; i < n; ++i)
        f();
}

void test_rb_single_thread()
{
    constexpr uint32_t numReaders = 1;
    RingBuffer1M rb(numReaders, 10, sizeof(foo), 1);

    constexpr uint32_t n = 18;
    constexpr uint32_t m = 12;

    do_n_times([&](){
            push_rb(rb); }, n);
    do_n_times([&](){
            pop_rb(rb); }, m);
    do_n_times([&](){
            push_rb(rb); }, n);
    do_n_times([&](){
            pop_rb(rb); }, m);
    do_n_times([&](){
            push_rb(rb); }, n);
    do_n_times([&](){
            pop_rb(rb); }, m);
    do_n_times([&](){
            push_rb(rb); }, n);
    do_n_times([&](){
            pop_rb(rb); }, m);

}


int main()
{
    //test_ba();
    test_rb();
    //test_rb_single_thread();
}

