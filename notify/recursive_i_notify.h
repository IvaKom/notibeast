#ifndef RECURSIVE_I_NOTIFY_H
#define RECURSIVE_I_NOTIFY_H

#include <functional>
#include <vector>
#include <string>
#include "filesystem.h"
#include "glue/message_provider.h"

struct RecursiveNotifyEvent;

class RecursiveINotify: public MessageProvider {
  class RecursiveINotifyImpl;
public:
  explicit RecursiveINotify(std::function<void(RecursiveNotifyEvent)>,
                            fs::path const &path,
                            std::vector<std::string> pathsToSkip = {});
  ~RecursiveINotify();
  RecursiveINotify(RecursiveINotify const &) = delete;
  RecursiveINotify& operator=(RecursiveINotify const&) = delete;
private:
  std::unique_ptr<RecursiveINotifyImpl> pImpl;
  void logFiltered(std::string const &ss, int filteringMask) const override;
  void logSubscribing(int mask) const override;
};

#endif
