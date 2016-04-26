#pragma once
#include <stdint.h>
#include <memory>
#include <atomic>


/** lockfree block allocator */
class BlockAllocator
{
    public:
        /**
         * Create block allocator wich contain numBlocks blocks
         * of sizeOfBlock size.  */
        BlockAllocator(size_t numBlocks, size_t sizeOfBlock);

        /** Get unused block.
         *  @return pointer to unused block or nullptr if there no free blocks*/
        void* get_nothrow();

        /**
         * Get unused block. If there no free block
         * then std::bad_alloc exception thrown
         * @return pointer to unused block. */
        void* get();

        /** Return previously allocated block. Must not be used any more.
         * @param p pointer to block to return. */
        void free(void* p);

    private:
        /** Array for storing numBlock elements 
         * each of size sizeOfBlock */ 
        std::unique_ptr<char, void(&)(char*)> data; 
                                                    
        /** Stack elements array */
        std::unique_ptr<uint32_t, void(&)(uint32_t*)> nodes;

        /** Size of each block */
        size_t sizeOfElement;

        /** Number of blocks */
        size_t numberOfElements;
        std::atomic_uint_least64_t  stack; 
};
