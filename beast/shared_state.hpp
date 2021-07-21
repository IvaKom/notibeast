// Tailored version of:
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef NOTIFYING_STATE_HPP
#define NOTIFYING_STATE_HPP

#include <mutex>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "glue/message_provider_factory.h"

class websocket_session;

class shared_state: public MessageSender {
    // This mutex synchronizes all access to sessions_
    mutable std::mutex mutex_;

    // Keep a list of all the connected clients and associated notification masks
    std::unordered_map<websocket_session*, uint64_t> sessions_;

    void send(std::string message, int mask) const override;

    std::unique_ptr<MessageProvider> messageProvider_;
public:
    explicit
    shared_state(const MessageProviderFactory &factory);
    ~shared_state() override;

    void join(websocket_session* session);
    void leave(websocket_session* session);
    void subscribe(websocket_session* session, uint64_t mask);
};

#endif
