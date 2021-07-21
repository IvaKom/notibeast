#ifndef RECURSIVE_I_NOTIFY_FACTORY_H
#define RECURSIVE_I_NOTIFY_FACTORY_H

#include "glue/message_provider_factory.h"
#include "glue/options.h"

class RecursiveINotifyFactory : public MessageProviderFactory {
public:
  explicit RecursiveINotifyFactory(Options options);
  ~RecursiveINotifyFactory() = default;
private:
  Options options;  
  std::unique_ptr<MessageProvider> makeMessageProvider(const MessageSender &messageSender) const override;
};

#endif
