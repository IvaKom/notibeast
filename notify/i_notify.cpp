#include "i_notify.h"

// Based on https://www.man7.org/linux/man-pages/man7/inotify.7.html#EXAMPLES

#include "i_notify_helper.h"

#include <boost/log/trivial.hpp>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <iostream>
#include <thread>
#include <poll.h>
#include <unistd.h>

namespace std {

std::ostream& operator<< (std::ostream &out, const NotifyEvent &ne) {
  out << "{wd: " << ne.wd
      << ", mask: \"" << strMask(ne.mask)
      << "\", cookie: " << ne.cookie
      << "\", name: \"" << ne.name
      << "\"}";
  return out;
}

} //namespace std

namespace {

// it throws on error
void handle_events(int fd, std::function<void(NotifyEvent)> fn) {

   // Some systems cannot read integer variables if they are not
   // properly aligned. On other systems, incorrect alignment may
   // decrease performance. Hence, the buffer used for reading from
   // the inotify file descriptor should have the same alignment as
   // struct inotify_event.

   char buf[4096]
       __attribute__ ((aligned(__alignof__(struct inotify_event))));

   // Loop while events can be read from inotify file descriptor.
   for (;;) {
     // Read some events.
     ssize_t len = read(fd, buf, sizeof(buf));
     if (len == -1 && errno != EAGAIN) {
       std::stringstream sstr;
       sstr<< "Can't read from file descriptor, error: " << strerror(errno);
       BOOST_LOG_TRIVIAL(error) << sstr.str();
       throw std::runtime_error(sstr.str());
     }

     // If the nonblocking read() found no events to read, then
     // it returns -1 with errno set to EAGAIN. In that case,
     // we exit the loop.
     if (len <= 0)
       break;

     // Loop over all events in the buffer
     const struct inotify_event *event;
     for (char *ptr = buf; ptr < buf + len;
          ptr += sizeof(struct inotify_event) + event->len)
     {
       event = reinterpret_cast<const struct inotify_event *>(ptr);
       auto ne = NotifyEvent{event};
       BOOST_LOG_TRIVIAL(debug) << "Passing event to the client-provided callback. " << ne;
       fn(ne);
     }// for events
   }
} //handle_events

} //namespace


INotify::INotify(std::function<void(NotifyEvent)> fn) {
  BOOST_LOG_TRIVIAL(info) << "Initializing inotify";

  efd = eventfd(0,0);

  fd = inotify_init1(IN_NONBLOCK);
  if (fd == -1) {
    std::stringstream sstr;
    sstr<< "inotify_init1 failed with error: " << strerror(errno);
    BOOST_LOG_TRIVIAL(error) << sstr.str();
    throw std::runtime_error(sstr.str());
  }

  int lfd = fd;
  th = std::thread([lfd, fn, this]() {
    // Prepare for polling
    const int nfds = 2;

    pollfd fds[2];

    // Inotify input
    fds[0].fd = lfd;
    fds[0].events = POLLIN;
    fds[1].fd = efd;
    fds[1].events = POLLIN;

    BOOST_LOG_TRIVIAL(info) << "Listening for events.";
    while (true) {
      BOOST_LOG_TRIVIAL(trace)<<"Entering poll";
      int poll_num = poll(fds, nfds, -1);
      BOOST_LOG_TRIVIAL(trace)<<"poll returned";
      if (poll_num == -1) {
        if (errno == EINTR) {
         continue;
        }

        std::stringstream sstr;
        sstr<< "Polling of file descriptors failed, error: " << strerror(errno);
        BOOST_LOG_TRIVIAL(error) << sstr.str();
        throw std::runtime_error(sstr.str());
      }
      if (poll_num > 0) {
        if (fds[1].revents & POLLIN) {
          BOOST_LOG_TRIVIAL(debug) << "exiting event received";
          break;
        } else if (fds[0].revents & POLLIN) {
          // Inotify events are available
          handle_events(lfd, fn);
        } else {
          BOOST_LOG_TRIVIAL(info) << "Unexpected result of polling. Check what it is."
            << " [0].events: " << fds[0].events
            << " [1].events: " << fds[1].events;
        }
      }
    } //while
  });
}

INotify::~INotify() {
  BOOST_LOG_TRIVIAL(info) << "Stopping the thread";

  uint64_t u = 1;
  auto s = write(efd, &u, sizeof(u));
  if (s!=sizeof(u)) {
    BOOST_LOG_TRIVIAL(error) << "Failed to write to event fd, error: " << strerror(errno);
  }
  BOOST_LOG_TRIVIAL(debug) << "Joining the thread";
  th.join();
  if (fd!=-1) {
    BOOST_LOG_TRIVIAL(debug) << "Closing file descriptor";
    close(fd);
  }
  close(efd);
  BOOST_LOG_TRIVIAL(info) << "INotify is destructed";
}

int INotify::monitorPath(fs::path const &path) {
  int wd = inotify_add_watch(fd, path.c_str(), IN_ALL_EVENTS);
  if (wd == -1) {
    std::stringstream sstr;
    sstr << "inotify_add_watch failed for path " + std::to_string(wd)
         << ", error: " << strerror(errno);
    BOOST_LOG_TRIVIAL(error) << sstr.str();
    throw std::runtime_error(sstr.str());
  }
  return wd;
}

void INotify::removeWatch(int wd) {
  int res = inotify_rm_watch(fd, wd);
  if (res == -1) {
    std::stringstream sstr;
    sstr << "inotify_rm_watch failed for watch descriptor " << wd
         << ", error: " << strerror(errno);
    BOOST_LOG_TRIVIAL(error) << sstr.str();
    throw std::runtime_error(sstr.str());
  }
}
