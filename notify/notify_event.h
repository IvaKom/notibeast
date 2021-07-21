#ifndef NOTIFY_EVENT_H
#define NOTIFY_EVENT_H

#include <cstdint>
#include <string>

struct inotify_event;

struct NotifyEvent {
  int wd;
  uint32_t mask;
  uint32_t cookie;
  std::string name;
  NotifyEvent(const inotify_event *inotifyEvent);
};
 
#endif
