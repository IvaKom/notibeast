#ifndef MESSAGE_PROVIDER_FACTORY_H
#define MESSAGE_PROVIDER_FACTORY_H

#include "message_provider.h"
#include "message_sender.h"

#include <memory>

class MessageProviderFactory {
public:
  virtual std::unique_ptr<MessageProvider> makeMessageProvider(const MessageSender &) const = 0;
  virtual ~MessageProviderFactory() = 0;
};

inline MessageProviderFactory::~MessageProviderFactory() = default;  

#endif
