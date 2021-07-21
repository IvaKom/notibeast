// Tailored version of:
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "websocket_session.hpp"
#include "shared_state.hpp"
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>

websocket_session::
websocket_session(
    tcp::socket&& socket,
    boost::shared_ptr<shared_state> const& state)
    : ws_(std::move(socket))
    , state_(state)
{
}

websocket_session::
~websocket_session() {
    BOOST_LOG_TRIVIAL(info) <<"Closing websocket";
    // Remove this session from the list of active sessions
    state_->leave(this);
    if (ws_.is_open()) {
        ws_.close(websocket::close_reason("Shutting down"));
    }
}

void
websocket_session::
fail(beast::error_code ec, char const* what) {
    if( ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
    {
        BOOST_LOG_TRIVIAL(debug) << what << ": " << ec.message();
    } else {
        BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
    }
}

void
websocket_session::
on_accept(beast::error_code ec) {
    // Handle the error, if any
    if(ec)
        return fail(ec, "accept");

    // Add this session to the list of active sessions
    state_->join(this);

    BOOST_LOG_TRIVIAL(info) <<"Websocket connection established";

    // Read a message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void
websocket_session::
on_read(beast::error_code ec, std::size_t) {
    // Handle the error, if any
    if(ec)
        return fail(ec, "read");

    auto message = beast::buffers_to_string(buffer_.data());
    BOOST_LOG_TRIVIAL(info) <<"Received a message: " << message;

    processMessage(message);

    // Send to all connections
    //state_->send(message);

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Read another message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void
websocket_session::
send(boost::shared_ptr<std::string const> const& ss) {
    // Post our work to the strand, this ensures
    // that the members of `this` will not be
    // accessed concurrently.

    net::post(
        ws_.get_executor(),
        beast::bind_front_handler(
            &websocket_session::on_send,
            shared_from_this(),
            ss));
}

void
websocket_session::
on_send(boost::shared_ptr<std::string const> const& ss) {
    // Always add to queue
    queue_.push_back(ss);

    // Are we already writing?
    if(queue_.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        beast::bind_front_handler(
            &websocket_session::on_write,
            shared_from_this()));
}

void
websocket_session::
on_write(beast::error_code ec, std::size_t) {
    // Handle the error, if any
    if(ec)
        return fail(ec, "write");

    // Remove the string from the queue
    queue_.erase(queue_.begin());

    // Send the next message if any
    if(! queue_.empty())
        ws_.async_write(
            net::buffer(*queue_.front()),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
}

void
websocket_session::
processMessage(boost::string_view message) {
  BOOST_LOG_TRIVIAL(info) << "Processing message: '" << message;

  try {
    auto messageValue = boost::json::parse(message);
    auto &messageObject = messageValue.as_object();
    BOOST_LOG_TRIVIAL(debug) << "messageObject is: " <<  messageObject << std::endl;
    auto commandvalue = messageObject["command"];
    if (commandvalue == nullptr) {
      BOOST_LOG_TRIVIAL(debug) << "\"command\" is not present on the object";
      return;
    }
    BOOST_LOG_TRIVIAL(debug) << "commandvalue is: " << commandvalue;
    auto &command = commandvalue.as_string();
    BOOST_LOG_TRIVIAL(debug) << "command is: " << command;
    if (command== "subscribe") {
      auto maskValue = messageObject["mask"];
      if (maskValue == nullptr) {
        BOOST_LOG_TRIVIAL(debug) << "\"mask\" is not present on the object";
        return;
      }
      auto &mask = maskValue.as_int64();
      state_->subscribe(this, mask);
    }
  } catch (std::exception const &ec) {
    BOOST_LOG_TRIVIAL(error) << "JSON processing failed: " << ec.what();
  }

}
