#include "recursive_i_notify_factory.h"

#include "notify_event_funcs.h"
#include "notify/recursive_i_notify.h"

RecursiveINotifyFactory::RecursiveINotifyFactory(Options options)
  : options{std::move(options)}
{}

std::unique_ptr<MessageProvider> 
RecursiveINotifyFactory::makeMessageProvider(const MessageSender &messageSender) const {
  return std::make_unique<RecursiveINotify>(
    [&messageSender](const RecursiveNotifyEvent &rne) {
      auto message = eventToString(rne) + "\n";
      messageSender.send(message, rne.mask); // mask is sent around so we don't have to parse the message again
    },
    options.pathToMonitor,
    options.pathsToExclude
  );
}
