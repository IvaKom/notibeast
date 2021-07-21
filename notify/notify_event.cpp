#include "notify_event.h"

#include <sys/inotify.h>

namespace {

std::string makeName(inotify_event const *e){
  if (e->len == 0) {
    return "";
  } else {
    return std::string(e->name);
  }
}

} //namespace
 
NotifyEvent::NotifyEvent(inotify_event const *e):
  wd{e->wd},
  mask{e->mask},
  cookie{e->cookie},
  name{makeName(e)}
{}
 
