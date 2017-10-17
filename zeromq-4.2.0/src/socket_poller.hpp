#ifndef __ZMQ_SOCKET_POLLER_HPP_INCLUDED__
#define __ZMQ_SOCKET_POLLER_HPP_INCLUDED__

#include "poller.hpp"

#if defined ZMQ_POLL_BASED_ON_POLL && !defined ZMQ_HAVE_WINDOWS
#include <poll.h>
#endif

#if defined ZMQ_HAVE_WINDOWS
#include "windows.hpp"
#else
#include <unistd.h>
#endif

#include <vector>
#include <algorithm>

#include "socket_base.hpp"
#include "signaler.hpp"

namespace zmq
{

    class socket_poller_t
    {
    public:
        socket_poller_t ();
        ~socket_poller_t ();

        typedef struct event_t
        {
            socket_base_t *socket;
            fd_t fd;
            void *user_data;
            short events;
        } event_t;

        int add (socket_base_t *socket, void *user_data, short events);
        int modify (socket_base_t *socket, short events);
        int remove (socket_base_t *socket);

        int add_fd (fd_t fd, void *user_data, short events);
        int modify_fd (fd_t fd, short events);
        int remove_fd (fd_t fd);

        int wait (event_t *event, int n_events, long timeout);

        inline int size (void) { return items.size (); };

        //  Return false if object is not a socket.
        bool check_tag ();

    private:
        int rebuild ();

        //  Used to check whether the object is a socket_poller.
        uint32_t tag;

        //  Signaler used for thread safe sockets polling
        signaler_t signaler;

        typedef struct item_t {
            socket_base_t *socket;
            fd_t fd;
            void *user_data;
            short events;
#if defined ZMQ_POLL_BASED_ON_POLL
            int  pollfd_index;
#endif
        } item_t;

        //  List of sockets
        typedef std::vector <item_t> items_t;
        items_t items;

        //  Does the pollset needs rebuilding?
        bool need_rebuild;

        //  Should the signaler be used for the thread safe polling?
        bool use_signaler;

        //  Size of the pollset
        int poll_size;

#if defined ZMQ_POLL_BASED_ON_POLL
        pollfd *pollfds;
#elif defined ZMQ_POLL_BASED_ON_SELECT
        fd_set pollset_in;
        fd_set pollset_out;
        fd_set pollset_err;
        zmq::fd_t maxfd;
#endif

        socket_poller_t (const socket_poller_t&);
        const socket_poller_t &operator = (const socket_poller_t&);
    };

}

#endif
