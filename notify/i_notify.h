#ifndef I_NOTIFY_H
#define I_NOTIFY_H

#include "notify_event.h"

#include <functional>
#include <thread>

#include "filesystem.h"

// Wrapper for inotify(7)
// https://www.man7.org/linux/man-pages/man7/inotify.7.html
class INotify {
public:
  // NOTE: the callback will be invoked from a different thread
  explicit INotify(std::function<void(NotifyEvent)>);
  INotify(INotify const &) = delete;
  INotify& operator=(INotify const&) = delete;
  ~INotify();

  int monitorPath(fs::path const &path); // return watch descriptor
  void removeWatch(int wd);
private:
  int fd = -1;  // file  descriptor for inotify
  int efd = -1; // event desriptor to exit waiting on "poll"
  std::thread th;
};

namespace std {
std::ostream& operator<< (std::ostream &out, const NotifyEvent &ne);
}
#endif
