#include "i_notify_helper.h"
#include <sys/inotify.h>
#include <sstream>

std::string
strMask(uint32_t mask) {
  std::string res;
  if (mask & IN_ACCESS) {
    res = "IN_ACCESS";                  mask &= ~IN_ACCESS;
  } else if (mask & IN_ATTRIB) {
    res = "IN_ATTRIB";                  mask &= ~IN_ATTRIB;
  } else if (mask & IN_CLOSE_WRITE) {
    res = "IN_CLOSE_WRITE";             mask &= ~IN_CLOSE_WRITE;
  } else if (mask & IN_CLOSE_NOWRITE) {
    res = "IN_CLOSE_NOWRITE";           mask &= ~IN_CLOSE_NOWRITE;
  } else if (mask & IN_DELETE) {
    res = "IN_DELETE";                  mask &= ~IN_DELETE;
  } else if (mask & IN_CREATE) {
    res = "IN_CREATE";                  mask &= ~IN_CREATE;
  } else if (mask & IN_DELETE_SELF) {
    res = "IN_DELETE_SELF";             mask &= ~IN_DELETE_SELF;
  } else if (mask & IN_MODIFY) {
    res = "IN_MODFIY";                  mask &= ~IN_MODIFY;
  } else if (mask & IN_MOVE_SELF) {
    res = "IN_MOVE_SELF";               mask &= ~IN_MOVE_SELF;
  } else if (mask & IN_MOVED_FROM) {
    res = "IN_MOVED_FROM";              mask &= ~IN_MOVED_FROM;
  } else if (mask & IN_MOVED_TO) {
    res = "IN_MOVED_TO";                mask &= ~IN_MOVED_TO;
  } else if (mask & IN_OPEN) {
    res = "IN_OPEN";                    mask &= ~IN_OPEN;
  } else if (mask & IN_IGNORED) {
    res = "IN_IGNORED";                 mask &= ~IN_IGNORED;
  } else if (mask & IN_UNMOUNT) {
    res = "IN_UNMOUNT";                 mask &= ~IN_UNMOUNT;
  } else if (mask & IN_Q_OVERFLOW) {
    res = "IN_Q_OVERFLOW";              mask &= ~IN_Q_OVERFLOW;
  } else if (mask & IN_ISDIR) {
    res = "IN_ISDIR";                   mask &= ~IN_ISDIR;
  }

  if (mask && res.empty()) {
    std::stringstream ss;
    ss << "Undetected mask: " << std::hex << std::showbase << mask;
    return ss.str();
  }
  return mask ? res + "|" + strMask(mask) : res;
} //strMask
