#ifndef MESSAGE_PROVIDER_H
#define MESSAGE_PROVIDER_H

#include <string>

class MessageProvider {
public:
  virtual ~MessageProvider () = 0;
  virtual void logFiltered(std::string const &ss, int filteringMask) const = 0;
  virtual void logSubscribing(int mask) const = 0;
};

inline MessageProvider::~MessageProvider() = default;

#endif
