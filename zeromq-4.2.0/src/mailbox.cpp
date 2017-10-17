#include "precompiled.hpp"
#include "mailbox.hpp"
#include "err.hpp"

zmq::mailbox_t::mailbox_t ()
{
    //  Get the pipe into passive state. That way, if the users starts by
    //  polling on the associated file descriptor it will get woken up when
    //  new command is posted.
    const bool ok = cpipe.read (NULL);
    zmq_assert (!ok);
    active = false;
}

zmq::mailbox_t::~mailbox_t ()
{
    //  TODO: Retrieve and deallocate commands inside the cpipe.

    // Work around problem that other threads might still be in our
    // send() method, by waiting on the mutex before disappearing.
    sync.lock ();
    sync.unlock ();
}

zmq::fd_t zmq::mailbox_t::get_fd () const
{
    return signaler.get_fd ();
}

void zmq::mailbox_t::send (const command_t &cmd_)
{
    sync.lock ();
    cpipe.write (cmd_, false);
    const bool ok = cpipe.flush ();
    sync.unlock ();
    if (!ok)
        signaler.send ();
}

int zmq::mailbox_t::recv (command_t *cmd_, int timeout_)
{
    //  Try to get the command straight away.
    if (active) {
        if (cpipe.read (cmd_))
            return 0;

        //  If there are no more commands available, switch into passive state.
        active = false;
    }

    //  Wait for signal from the command sender.
    int rc = signaler.wait (timeout_);
    if (rc == -1) {
        errno_assert (errno == EAGAIN || errno == EINTR);
        return -1;
    }

    //  Receive the signal.
    rc = signaler.recv_failable ();
    if (rc == -1) {
        errno_assert (errno == EAGAIN);
        return -1;
    }

    //  Switch into active state.
    active = true;

    //  Get a command.
    const bool ok = cpipe.read (cmd_);
    zmq_assert (ok);
    return 0;
}
