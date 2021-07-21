#include "notify_event_funcs.h"
#include "notify/i_notify_helper.h"
#include <boost/json/src.hpp>
#include <sstream>

namespace js=boost::json;

void tag_invoke(js::value_from_tag, js::value &jv, const RecursiveNotifyEvent &event) {
  jv = {
    {"path", event.path},
    {"name", event.name},
    {"mask", event.mask},
    {"cookie", event.cookie}
  };
}

std::string eventToString(const RecursiveNotifyEvent &event) {
  std::stringstream message;
  message << js::serialize(js::value_from(event));
  return message.str();
}
