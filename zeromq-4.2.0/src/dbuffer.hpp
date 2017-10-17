#ifndef __ZMQ_DBUFFER_HPP_INCLUDED__
#define __ZMQ_DBUFFER_HPP_INCLUDED__

#include <stdlib.h>
#include <stddef.h>
#include <algorithm>

#include "mutex.hpp"
#include "msg.hpp"

namespace zmq
{

    //  dbuffer is a single-producer single-consumer double-buffer
    //  implementation.
    //
    //  The producer writes to a back buffer and then tries to swap
    //  pointers between the back and front buffers. If it fails,
    //  due to the consumer reading from the front buffer, it just
    //  gives up, which is ok since writes are many and redundant.
    //
    //  The reader simply reads from the front buffer.
    //
    //  has_msg keeps track of whether there has been a not yet read
    //  value written, it is used by ypipe_conflate to mimic ypipe
    //  functionality regarding a reader being asleep

    template <typename T> class dbuffer_t;

    template <> class dbuffer_t<msg_t>
    {
    public:

        inline dbuffer_t ()
            : back (&storage[0])
            , front (&storage[1])
            , has_msg (false)
        {
            back->init ();
            front->init ();
        }

        inline ~dbuffer_t()
        {
            back->close ();
            front->close ();
        }

        inline void write (const msg_t &value_)
        {
            msg_t& xvalue = const_cast<msg_t&>(value_);

            zmq_assert (xvalue.check ());
            back->move (xvalue);    // cannot just overwrite, might leak

            zmq_assert (back->check ());

            if (sync.try_lock ())
            {
                std::swap (back, front);
                has_msg = true;

                sync.unlock ();
            }
        }

        inline bool read (msg_t *value_)
        {
            if (!value_)
                return false;

            {
                scoped_lock_t lock (sync);
                if (!has_msg)
                    return false;

                zmq_assert (front->check ());

                *value_ = *front;
                front->init ();     // avoid double free

                has_msg = false;
                return true;
            }
        }


        inline bool check_read ()
        {
            scoped_lock_t lock (sync);

            return has_msg;
        }

        inline bool probe (bool (*fn)(const msg_t &))
        {
            scoped_lock_t lock (sync);
            return (*fn) (*front);
        }


    private:
        msg_t storage[2];
        msg_t *back, *front;

        mutex_t sync;
        bool has_msg;

        //  Disable copying of dbuffer.
        dbuffer_t (const dbuffer_t&);
        const dbuffer_t &operator = (const dbuffer_t&);
    };
}

#endif
