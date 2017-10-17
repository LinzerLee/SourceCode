#ifndef __ZMQ_YPIPE_HPP_INCLUDED__
#define __ZMQ_YPIPE_HPP_INCLUDED__

#include "atomicp_tr.hpp"
#include "yqueue.hpp"
#include "ypipe_base.hpp"

namespace zmq
{

    //  Lock-free queue implementation.
    //  Only a single thread can read from the pipe at any specific moment.
    //  Only a single thread can write to the pipe at any specific moment.
    //  T is the type of the object in the queue.
    //  N is granularity of the pipe, i.e. how many items are needed to
    //  perform next memory allocation.
    //  #!#总结:
    //  1. r(front | w)追着c(front | w), c(null | w)追着w(上次f | 本次f), w追着f
    //  2. 对于读者线程(check_read)来说,c的值只会是front或w,不会为空,因为null
    //      的赋值是由读者线程来做的,此时一定是写者唤醒后的.
    //      当r!=front时,说明此时 预读指针r已经移动过了,可以直接读.
    //      当弹出操作已经将预读的都读完了,即r==front,且预读 指针
    //      能再往后移,因为r==c,所以c==front,故此时读者可以休眠了,
    //      c赋值为null
    //  3. 对于写者线程(flush)来说,c的值只会是w或null,c为null是由2造成的,
    //      此时应该让c指向上次刷新的位置w,并唤醒读者,若果不为
    //      null,说明读者已经是active状态,那么将w和c都指向本次刷新的位置f
    template <typename T, int N> class ypipe_t : public ypipe_base_t <T>
    {
    public:

        //  Initialises the pipe.
        inline ypipe_t ()
        {
            //  Insert terminator element into the queue.
            queue.push ();

            //  Let all the pointers to point to the terminator.
            //  (unless pipe is dead, in which case c is set to NULL).
            r = w = f = &queue.back ();
            c.set (&queue.back ());
        }

        //  The destructor doesn't have to be virtual. It is made virtual
        //  just to keep ICC and code checking tools from complaining.
        inline virtual ~ypipe_t ()
        {
        }

        //  Following function (write) deliberately copies uninitialised data
        //  when used with zmq_msg. Initialising the VSM body for
        //  non-VSM messages won't be good for performance.

#ifdef ZMQ_HAVE_OPENVMS
#pragma message save
#pragma message disable(UNINIT)
#endif

        //  Write an item to the pipe.  Don't flush it yet. If incomplete is
        //  set to true the item is assumed to be continued by items
        //  subsequently written to the pipe. Incomplete items are never
        //  flushed down the stream.
        inline void write (const T &value_, bool incomplete_)
        {
            //  Place the value to the queue, add new terminator element.
            queue.back () = value_;
            queue.push ();

            //  Move the "flush up to here" poiter.
            if (!incomplete_)
                f = &queue.back ();
        }

#ifdef ZMQ_HAVE_OPENVMS
#pragma message restore
#endif

        //  Pop an incomplete item from the pipe. Returns true if such
        //  item exists, false otherwise.
        inline bool unwrite (T *value_)
        {
            if (f == &queue.back ())
                return false;
            queue.unpush ();
            *value_ = queue.back ();
            return true;
        }

        //  Flush all the completed items into the pipe. Returns false if
        //  the reader thread is sleeping. In that case, caller is obliged to
        //  wake the reader up before using the pipe again.
        //  #!#flush的返回值不是是否失败,而是告诉调用者reader
        //  是否是sleep状态
        //  f--指向被刷新的最大位置
        //  w--指向上一次被刷新的位置,同时用来和c比对,如果
        //  值一样说明reader处于运行状态,否则c一定是null,reader
        //  处于sleep状态
        inline bool flush ()
        {
            //  If there are no un-flushed items, do nothing.
            if (w == f)
                return true;

            //  Try to set 'c' to 'f'.
            if (c.cas (w, f) != w) {

                //  Compare-and-swap was unseccessful because 'c' is NULL.
                //  This means that the reader is asleep. Therefore we don't
                //  care about thread-safeness and update c in non-atomic
                //  manner. We'll return false to let the caller know
                //  that reader is sleeping.
                c.set (f);
                w = f;
                return false;
            }

            //  Reader is alive. Nothing special to do now. Just move
            //  the 'first un-flushed item' pointer to 'f'.
            w = f;
            return true;
        }

        //  Check whether item is available for reading.
        //  #!#注意: check read时,c一定不为null
        //  c--表示可被预读取的最后的位置,如果c==front表示
        //  没有数据可被预读,更不能进行实际的读取,同时调
        //  整r指向front,c指向null,reader睡眠
        //  r--表示预读取的位置,一般指向front或c的位置
        //  r!=front说明已经预取一部分了,可以进行这部分的
        //  读取
        inline bool check_read ()
        {
            //  Was the value prefetched already? If so, return.
            if (&queue.front () != r && r)
                 return true;

            //  There's no prefetched value, so let us prefetch more values.
            //  Prefetching is to simply retrieve the
            //  pointer from c in atomic fashion. If there are no
            //  items to prefetch, set c to NULL (using compare-and-swap).
            // 此时的c要么是front,要么是w,肯定不会是null
            r = c.cas (&queue.front (), NULL);

            //  If there are no elements prefetched, exit.
            //  During pipe's lifetime r should never be NULL, however,
            //  it can happen during pipe shutdown when items
            //  are being deallocated.
            if (&queue.front () == r || !r)
                return false;

            //  There was at least one value prefetched.
            return true;
        }

        //  Reads an item from the pipe. Returns false if there is no value.
        //  available.
        inline bool read (T *value_)
        {
            //  Try to prefetch a value.
            if (!check_read ())
                return false;

            //  There was at least one value prefetched.
            //  Return it to the caller.
            *value_ = queue.front ();
            queue.pop ();
            return true;
        }

        //  Applies the function fn to the first elemenent in the pipe
        //  and returns the value returned by the fn.
        //  The pipe mustn't be empty or the function crashes.
        inline bool probe (bool (*fn)(const T &))
        {
            bool rc = check_read ();
            zmq_assert (rc);

            return (*fn) (queue.front ());
        }

    protected:

        //  Allocation-efficient queue to store pipe items.
        //  Front of the queue points to the first prefetched item, back of
        //  the pipe points to last un-flushed item. Front is used only by
        //  reader thread, while back is used only by writer thread.
        yqueue_t <T, N> queue;

        //  Points to the first un-flushed item. This variable is used
        //  exclusively by writer thread.
        T *w;

        //  Points to the first un-prefetched item. This variable is used
        //  exclusively by reader thread.
        T *r;

        //  Points to the first item to be flushed in the future.
        T *f;

        //  The single point of contention between writer and reader thread.
        //  Points past the last flushed item. If it is NULL,
        //  reader is asleep. This pointer should be always accessed using
        //  atomic operations.
        atomic_ptr_t <T> c;

        //  Disable copying of ypipe object.
        ypipe_t (const ypipe_t&);
        const ypipe_t &operator = (const ypipe_t&);
    };

}

#endif
