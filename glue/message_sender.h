#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include <string>

class MessageSender {
protected:
  virtual ~MessageSender() = 0;
public:
  virtual void send(std::string message, int mask) const = 0;
};

inline MessageSender::~MessageSender() = default;

#endif
