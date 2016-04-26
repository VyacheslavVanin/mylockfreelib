#pragma once
#include "block_allocator.hpp"

template<typename T>
class ObjectPool
{
    public:
        ObjectPool(size_t numObjects):ba(numObjects, sizeof(T)){} 

        template<typename... Args>
        T* get(Args&&... args) {
            void* p = ba.get();
            new(p)T(args...);
            return reinterpret_cast<T*>(p);
        }

        void free(T* p) {
            p->~T();
            ba.free(p);
        }

    private:
        BlockAllocator ba;
};

