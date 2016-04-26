#include "block_allocator.hpp"
#include <limits>

using stacknode=uint32_t;

typedef struct {
    union {
        struct {
            uint32_t       top;  /* index of top element */
            uint32_t      counter;
        };
        uint64_t  whole;
    };
}lfstack;

enum{STACK_EMPTY=std::numeric_limits<uint32_t>::max()};

static void deleter(char* p) {::free(p);}
static void nodedeleter(stacknode* p) {::free(p);}


#define SIZE_GRANULARITY sizeof(double)

static size_t getAllignedSize(size_t size, size_t align)
{
    return (size/align + (size%align != 0))*align; 
}

BlockAllocator::BlockAllocator(size_t numBlocks, size_t sizeOfBlock)
          : data( (char*)calloc(numBlocks,
                                getAllignedSize(sizeOfBlock, SIZE_GRANULARITY)),
                  deleter),
            nodes( (stacknode*)calloc(numBlocks, sizeof(stacknode)),
                    nodedeleter),
            sizeOfElement(sizeOfBlock),
            numberOfElements(numBlocks),
            stack()
{
    lfstack stackval;
    stackval.counter = 0;
    stackval.top = STACK_EMPTY;

    for(size_t i = 0; i < numBlocks; ++i)
        nodes.get()[i] = i+1;

    nodes.get()[ numBlocks-1 ] = STACK_EMPTY;
    stackval.top = 0;
    stack = stackval.whole;
}

void* BlockAllocator::get() {
    auto p = get_nothrow();
    if(p==nullptr)
        throw std::bad_alloc();
    return p;
}

void* BlockAllocator::get_nothrow() {
    lfstack    oldval;
    lfstack    newval;

    do {
        oldval.whole = stack.load();
        if(oldval.top == STACK_EMPTY)
            return nullptr;

        newval.counter = oldval.counter + 1;
        newval.top     = nodes.get()[ oldval.top ];
    }while(!stack.compare_exchange_strong(oldval.whole, newval.whole));

    return &data.get()[oldval.top*sizeOfElement];
}

void BlockAllocator::free(void* p)
{
    const char* const datastart = (const char*)data.get();
    const char* const endofdata = datastart + numberOfElements * (sizeOfElement);

    /* хорошо бы проверить еще, что укзатель правильно выравнен или типа того*/
    if( (p >= (void*)endofdata) || (p < (void*)datastart) )
        throw std::logic_error("invalid free. Pointer is not from this allocator.");

    const size_t index = ( (const char*)p - datastart )/sizeOfElement;

    lfstack    oldval;
    lfstack    newval;
    stacknode* node = &nodes.get()[ index ];
    do {
        oldval.whole = stack.load();
        newval.top = index;
        newval.counter = oldval.counter + 1;
        *node = oldval.top;
    }while(!stack.compare_exchange_strong(oldval.whole, newval.whole));
}


