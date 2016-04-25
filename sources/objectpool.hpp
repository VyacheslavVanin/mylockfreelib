#pragma once
#include <stdint.h>
#include <memory>
#include <atomic>
#include <limits>


/* lockfree allocator structure */
class blockAllocator
{
    public:
        blockAllocator(size_t numBlocks, size_t sizeOfBlock)
          : data( (char*)calloc(numBlocks, sizeOfBlock), deleter),
            nodes( (stacknode*)calloc(numBlocks, sizeof(stacknode)),
                    nodedeleter),
            sizeOfElement(sizeOfBlock),
            numberOfElements(numBlocks),
            stack()
        {
            lfstack_ stackval;
            stackval.counter = 0;
            stackval.top = STACK_EMPTY;

            for(size_t i = 0; i < numBlocks; ++i)
                nodes.get()[i].next = i+1;

            nodes.get()[ numBlocks-1 ].next = STACK_EMPTY;
            stackval.top = 0;
            stack = stackval.whole;
        }

        void* get_nothrow() {
            lfstack_    oldval;
            lfstack_    newval;

            do {
                oldval.whole = stack.load();
                if(oldval.top == STACK_EMPTY)
                    return nullptr;

                newval.counter = oldval.counter + 1;
                newval.top     = nodes.get()[ oldval.top ].next;
            }while(!stack.compare_exchange_strong(oldval.whole, newval.whole));

            void* ptr = &data.get()[oldval.top];
            return ptr;
        }

        void* get() {
            auto p = get_nothrow();
            if(p==nullptr)
                throw std::bad_alloc();
            return p;
        }

        void free(void* p) {
            const char* const datastart = (const char*)data.get();
            const char* const endofdata = datastart + numberOfElements * (sizeOfElement);

            /* хорошо бы проверить еще, что укзатель правильно выравнен или типа того*/
            if( (p >= (void*)endofdata) || (p < (void*)datastart) )
                throw std::logic_error("invalid free. Pointer is not from this allocator.");

            const size_t index = ( (const char*)p - datastart )/sizeOfElement;

            lfstack_    oldval;
            lfstack_    newval;
            stacknode* node = &nodes.get()[ index ];
            do {
                oldval.whole = stack.load();
                newval.top = index;
                newval.counter = oldval.counter + 1;
                node->next = oldval.top;
            }while(!stack.compare_exchange_strong(oldval.whole, newval.whole));
        }

    private:
        struct stacknode {
            int32_t next;
        };

        typedef struct {
            union {
                struct {
                    uint32_t       top;  /* index of top element */
                    uint32_t      counter;
                };
                uint64_t  whole;
            };
        }lfstack_;
        enum{STACK_EMPTY=std::numeric_limits<uint32_t>::max()};

        static void deleter(char* p) {::free(p);}
        static void nodedeleter(stacknode* p) {::free(p);}
        std::unique_ptr<char, void(&)(char*)> data; /* array for storing numBlock elements 
                                     * each of size sizeOfBlock */ 
        std::unique_ptr<stacknode, void(&)(stacknode*)> nodes; /* stack elements array */
        size_t sizeOfElement; /* size of each block */
        size_t numberOfElements; /* number of blocks */
        std::atomic_uint_least64_t  stack; 
};

template<typename T>
class ObjectPool
{
    public:
        ObjectPool(size_t numObjects):ba(numObjects, sizeof(T)){} 

        template<typename... Args>
        T* get(Args&&... args)
        {
            void* p = ba.get();
            new(p)T(args...);
            return reinterpret_cast<T*>(p);
        }

        void free(T* p)
        {
            ba.free(p);
        }

    private:
        blockAllocator ba;
};

