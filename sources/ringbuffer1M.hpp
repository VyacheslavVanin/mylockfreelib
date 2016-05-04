#pragma once
#include "block_allocator.hpp"

struct lfcq_node{void* data;};
static void lfcq_node_deleter(lfcq_node* p) {free(p);}

class RingBuffer1M
{
    public:
        RingBuffer1M(size_t numReaders,
                     size_t length, size_t blockSize,
                     size_t additionalBlocks=0)
            : size(length+1),
              ba(size+numReaders+additionalBlocks+1, blockSize),
              mas((lfcq_node*)calloc(size,sizeof(lfcq_node)), lfcq_node_deleter),
              head(0),tail(0)
        {}
        
        void* getBlank()         { return ba.get(); }
        void* getBlank_nothrow() { return ba.get_nothrow(); }

        /** @brief Write filled blank to queue.
         *  @return if queue was overflowed then pointer to data wich was removed from queue returned
         *          else return nullptr */
        void* pushBlank_ext(void* blank)
        {
            void* ret = nullptr;
            uint32_t        oldHead;
            uint32_t        oldTail;
            uint32_t        headindex = 0;

            /* проверка на заполненость очереди */
            oldHead = head;
            oldTail = tail.load();

            /* вычисление следующего значения head */
            if( oldHead + 1 > oldHead ) /* если нет перехода через граицу типа */
                headindex = oldHead + 1;
            else                        /* если при инкрементировании произойдет переход через границу типа */
                headindex = (oldHead % size)  + 1 ;

            /*если очередь заполнена*/
            if( (oldTail % size) == ( headindex % size) ) /*!!!*/ 
                    ret = pop_ext();/* то надо удалить (и освободить) один элемент из очереди  */

            mas.get()[ (oldHead % size) ].data = blank; /* можно установить, т.к.
                                                         * pop сюда не дотянется т.к. у него
                                                         * еще хэд = тэйлу                */
            /* в случае 1 - 1 тут никак и неизменятся указатели
             *     здесь надо просто одновременно изменить оба индекса(tail и head)  */
            head = headindex;
            return ret;
        }

        /**
         * @brief Same as pushBlank_ext but auto release pushed out element.
         * */
        void pushBlank(void* blank)
        {
            auto pp = pushBlank_ext(blank);
            if(pp) release(pp);
        }

        /**
         * @brief pop data from queue.
         * @return pointer to removed data.
         * This pointer must be returned via this->release() */
        void* pop_ext()
        {
            void	   *ret = 0;   /*блок который надо будет освободить*/

            uint32_t    oldHead;
            uint32_t    oldTail;
            uint32_t    tailindex = 0;

            do {
                oldTail = tail.load(); /* порядок очень важен!!!*/
                oldHead = head;

                if( (oldTail % size) == (oldHead % size ) )
                    return nullptr;

                if( oldTail + 1 > oldTail )
                    tailindex = oldTail + 1;
                else
                    tailindex = (oldTail % size)  + 1 ;

                /*запоминаем блок для освобождения*/
                ret = mas.get()[ (oldTail % size) ].data;
            }while(!tail.compare_exchange_strong(oldTail, tailindex));

            return ret;
        }

        void release(void* node) { ba.free(node); }


    private:
        size_t size;
        BlockAllocator ba;
        std::unique_ptr<lfcq_node,void(&)(lfcq_node*)> mas;
        volatile uint32_t head;
        uint32_t    padding[15]; 
        std::atomic<uint32_t> tail;
};

