#pragma once
#include "block_allocator.hpp"


class RingBuffer1M
{
    public:
        RingBuffer1M(size_t numReaders,
                     size_t length, size_t blockSize,
                     size_t additionalBlocks=0);
        
        /**
         * @brief Get blank for writing data.
         * pushBlank/pushBlank_ext method should be called after writing data.
         * Then pointer should not be used.
         * Exception std::bad_alloc throw if therre no free blocks
         * (You can allocate more in constructor, or check
         * is every getBlank ends with pushBlank/pushBlank_ext or release, and
         * is every pop_ext ends with release )*/
        void* getBlank();

        /**
         * @brief  Get blank for writing data.
         * Same as getBlank but return nullptr if there no free blocks. */
        void* getBlank_nothrow();

        /** @brief Write filled blank to queue.
         *  @return if queue was overflowed then pointer to data wich was removed from queue returned
         *          else return nullptr */
        void* pushBlank_ext(void* blank);

        /**
         * @brief Same as pushBlank_ext but auto release pushed out element. */
        void pushBlank(void* blank);

        /**
         * @brief pop data from queue.
         * @return pointer to removed data.
         * This pointer must be returned via this->release() */
        void* pop_ext();

        /**
         * @brief Release pointer after pop_ext. */
        void release(void* node);

    private:
        size_t size;
        BlockAllocator ba;
        std::unique_ptr<void*,void(&)(void**)> mas;
        volatile uint32_t head;
        uint32_t    padding[15]; 
        std::atomic<uint32_t> tail;
};

