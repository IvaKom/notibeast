// Modified version of:
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "listener.hpp"
#include "http_session.hpp"
#include <iostream>
#include <boost/log/trivial.hpp>

namespace {

void fail(beast::error_code ec, char const* what) {
    // Don't report on canceled operations
    if(ec == net::error::operation_aborted) {
      BOOST_LOG_TRIVIAL(info) << what << ": " << ec.message();
    } else {
      BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
    }
}

} //namespace

listener::
listener(
    net::io_context& ioc,
    tcp::endpoint endpoint,
    boost::shared_ptr<shared_state> const& state)
    : ioc_(ioc)
    , acceptor_(ioc)
    , state_(state)
{
    BOOST_LOG_TRIVIAL(debug) << "Initialize listener";
    beast::error_code ec;

    BOOST_LOG_TRIVIAL(debug) << "Open the acceptor";
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) {
        fail(ec, "open");
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "Allow address reuse";
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) {
        fail(ec, "set_option");
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "Bind to the server address";
    acceptor_.bind(endpoint, ec);
    if(ec) {
        fail(ec, "bind");
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "Start listening for connections";
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if(ec) {
        fail(ec, "listen");
        return;
    }
}
listener::~listener() {
  BOOST_LOG_TRIVIAL(debug) << "listener is being destructed";
}

void
listener::
run() { // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &listener::on_accept,
            shared_from_this()));
}


// Handle a connection
void
listener::
on_accept(beast::error_code ec, tcp::socket socket)
{
    if(ec)
        return fail(ec, "accept");
    else {
        auto clientIp = socket.remote_endpoint().address().to_string();
        auto clientPort = socket.remote_endpoint().port();
        BOOST_LOG_TRIVIAL(info) << "Accepting connection from " << clientIp<< ":" <<clientPort; 
        // Launch a new session for this connection
        boost::make_shared<http_session>(
            std::move(socket),
            state_)->run();
    }

    // TODO: 'run' instead?

    // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &listener::on_accept,
            shared_from_this()));
}
