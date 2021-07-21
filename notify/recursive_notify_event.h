#ifndef RECURSIVE_NOTIFY_EVENT_H
#define RECURSIVE_NOTIFY_EVENT_H

#include <string>

struct NotifyEvent;

struct RecursiveNotifyEvent {
  uint32_t mask;
  uint32_t cookie;
  std::string path;
  std::string name;
};
 
#endif
