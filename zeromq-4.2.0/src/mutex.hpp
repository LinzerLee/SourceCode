#ifndef __ZMQ_MUTEX_HPP_INCLUDED__
#define __ZMQ_MUTEX_HPP_INCLUDED__

#include "err.hpp"

//  Mutex class encapsulates OS mutex in a platform-independent way.

#ifdef ZMQ_HAVE_WINDOWS

#include "windows.hpp"

namespace zmq
{

    class mutex_t
    {
    public:
        inline mutex_t ()
        {
            InitializeCriticalSection (&cs);
        }

        inline ~mutex_t ()
        {
            DeleteCriticalSection (&cs);
        }

        inline void lock ()
        {
            EnterCriticalSection (&cs);
        }

        inline bool try_lock ()
        {
            return (TryEnterCriticalSection (&cs)) ? true : false;
        }

        inline void unlock ()
        {
            LeaveCriticalSection (&cs);
        }

        inline CRITICAL_SECTION* get_cs()
        {
            return &cs;
        }

    private:

        CRITICAL_SECTION cs;

        //  Disable copy construction and assignment.
        mutex_t (const mutex_t&);
        void operator = (const mutex_t&);
    };

}

#else

#include <pthread.h>

namespace zmq
{

    class mutex_t
    {
    public:
        inline mutex_t ()
        {
            int rc = pthread_mutexattr_init(&attr);
            posix_assert (rc);

            rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            posix_assert (rc);

            rc = pthread_mutex_init (&mutex, &attr);
            posix_assert (rc);
        }

        inline ~mutex_t ()
        {
            int rc = pthread_mutex_destroy (&mutex);
            posix_assert (rc);

            rc = pthread_mutexattr_destroy (&attr);
            posix_assert (rc);
        }

        inline void lock ()
        {
            int rc = pthread_mutex_lock (&mutex);
            posix_assert (rc);
        }

        inline bool try_lock ()
        {
            int rc = pthread_mutex_trylock (&mutex);
            if (rc == EBUSY)
                return false;

            posix_assert (rc);
            return true;
        }

        inline void unlock ()
        {
            int rc = pthread_mutex_unlock (&mutex);
            posix_assert (rc);
        }

        inline pthread_mutex_t* get_mutex()
        {
            return &mutex;
        }

    private:

        pthread_mutex_t mutex;
        pthread_mutexattr_t attr;

        // Disable copy construction and assignment.
        mutex_t (const mutex_t&);
        const mutex_t &operator = (const mutex_t&);
    };

}

#endif


namespace zmq
{
    struct scoped_lock_t
    {
        scoped_lock_t (mutex_t& mutex_)
            : mutex (mutex_)
        {
            mutex.lock ();
        }

        ~scoped_lock_t ()
        {
            mutex.unlock ();
        }

    private:

        mutex_t& mutex;

        // Disable copy construction and assignment.
        scoped_lock_t (const scoped_lock_t&);
        const scoped_lock_t &operator = (const scoped_lock_t&);
    };


    struct scoped_optional_lock_t
    {
        scoped_optional_lock_t (mutex_t* mutex_)
            : mutex (mutex_)
        {
            if(mutex != NULL)
                mutex->lock ();
        }

        ~scoped_optional_lock_t ()
        {
            if(mutex != NULL)
                mutex->unlock ();
        }

    private:

        mutex_t* mutex;

        // Disable copy construction and assignment.
        scoped_optional_lock_t (const scoped_lock_t&);
        const scoped_optional_lock_t &operator = (const scoped_lock_t&);
    };


}

#endif
