#include "ringbuffer1M.hpp"

static void lfcq_node_deleter(void** p) {free(p);}
using lfcq_node = void*;

RingBuffer1M::RingBuffer1M(size_t numReaders,
                     size_t length, size_t blockSize,
                     size_t additionalBlocks)
            : size(length+1),
              ba(size+numReaders+additionalBlocks+1, blockSize),
              mas((lfcq_node*)calloc(size,sizeof(lfcq_node)), lfcq_node_deleter),
              head(0),tail(0)
        {}

void* RingBuffer1M::getBlank() {return ba.get();}
void* RingBuffer1M::getBlank_nothrow() {return ba.get_nothrow();}

void* RingBuffer1M::pushBlank_ext(void* blank) {
    void*    ret = nullptr;
    uint32_t headindex = 0;

    /* проверка на заполненость очереди */
    uint32_t oldHead = head;
    uint32_t oldTail = tail.load();

    /* вычисление следующего значения head */
    if( oldHead + 1 > oldHead ) /* если нет перехода через граицу типа */
        headindex = oldHead + 1;
    else                        /* если при инкрементировании произойдет переход через границу типа */
        headindex = (oldHead % size)  + 1 ;

    /*если очередь заполнена*/
    if( (oldTail % size) == ( headindex % size) ) /*!!!*/ 
        ret = pop_ext();/* то надо удалить (и освободить) один элемент из очереди  */

    mas.get()[ (oldHead % size) ] = blank; /* можно установить, т.к.
                                            * pop сюда не дотянется т.к. у него
                                            * еще хэд = тэйлу                */
    /* в случае 1 - 1 тут никак и неизменятся указатели
     *     здесь надо просто одновременно изменить оба индекса(tail и head)  */
    head = headindex;
    return ret;
}

void RingBuffer1M::pushBlank(void* blank)
{
    auto pp = pushBlank_ext(blank);
    if(pp) release(pp);
}

void* RingBuffer1M::pop_ext()
{
    void*    ret = 0;   /*блок который надо будет освободить*/
    uint32_t oldHead;
    uint32_t oldTail;
    uint32_t tailindex = 0;

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
        ret = mas.get()[ (oldTail % size) ];
    }while(!tail.compare_exchange_strong(oldTail, tailindex));

    return ret;
}

void RingBuffer1M::release(void* node)
{
    ba.free(node);
}


