#include "i_notify.h"

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <sys/inotify.h>
#include "helper.h"
#include <mutex>
#include <condition_variable>

#include "glue/options.h"
#include "beast/service.hpp"

#include <signal.h>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/json.hpp>

using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace js = boost::json;
using tcp = boost::asio::ip::tcp;

//Would have to inject the factory into runService() to avoid globals
std::mutex g_mtx;
std::condition_variable g_cv;
bool g_ready = false;

bool isFilteredOut = false;

class TestMessageProvider: public MessageProvider {
public:
  TestMessageProvider(const MessageSender &messageSender) {
    std::thread([&messageSender](){
      BOOST_LOG_TRIVIAL(info) << "Waiting for the connection to be established";
      std::unique_lock<std::mutex> lck(g_mtx);
      while (!g_ready) g_cv.wait_for(lck, milliseconds(100));
      BOOST_LOG_TRIVIAL(info) << "Connection is established, sending messages";
      messageSender.send("Message 153, to be filtered out\n", IN_ACCESS);
      messageSender.send("Message 42\n", IN_CLOSE_WRITE);
    }).detach();
  }
private:
  void logFiltered( [[maybe_unused]] std::string const &ss, [[maybe_unused]] int filteringMask) const override {
    std::string_view sv = ss;
    if (!sv.empty() && *sv.rbegin() == '\n') {
      sv.remove_suffix(1);
    }
    BOOST_LOG_TRIVIAL(debug) << "Message '" << sv << "' is filtered out by mask " << filteringMask << "";
    if (ss == "Message 153, to be filtered out\n") {
      isFilteredOut = true;
    }
  }
  void logSubscribing( [[maybe_unused]] int mask) const override {}
};

class TestFactory: public MessageProviderFactory {
public:
  TestFactory( [[maybe_unused]] Options options)
  {}

  std::unique_ptr<MessageProvider> makeMessageProvider(const MessageSender &messageSender) const override {
    return std::make_unique<TestMessageProvider>(messageSender);
  }
};

void connectToServer(tcp::resolver &resolver,
  websocket::stream<tcp::socket> &ws)
{
  BOOST_LOG_TRIVIAL(info) << "Look up the domain name\n";
  std::string host = "localhost";
  auto const results = resolver.resolve(host, "8080");

  BOOST_LOG_TRIVIAL(info) << "Make the connection on the IP address we get from a lookup";
  for(int i=1; i<=10; ++i) {
    boost::system::error_code ec;
    auto ep = net::connect(ws.next_layer(), results, ec);
    if (ec) {
      BOOST_LOG_TRIVIAL(debug) << "connection to the server failed: " << ec.message();
      sleep_for(milliseconds(i*i*10));
    } else {
      BOOST_LOG_TRIVIAL(info) << "Update the host_ string.\n";
      // This will provide the value of the
      // Host HTTP header during the WebSocket handshake.
      // See https://tools.ietf.org/html/rfc7230#section-5.4
      host += ':' + std::to_string(ep.port());

      // Set a decorator to change the User-Agent of the handshake
      ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-sync");
        }));

      ws.handshake(host, "/");

      BOOST_LOG_TRIVIAL(info) << "websocket connection to the server established";
      return;
    }
  }
  throw std::runtime_error("Failed to establish connection");
}

void subscribe(websocket::stream<tcp::socket> &ws, int mask) {
  std::cout << "subscribing to " << mask << " events\n";
  js::object cmd;
  cmd.emplace("command", "subscribe");
  cmd.emplace("mask", mask);
  ws.write(net::buffer(js::serialize(cmd)));
}

std::string readLine(websocket::stream<tcp::socket> &ws) {
  // Read a message into our buffer
  // TODO: timeout? async?
  boost::asio::streambuf sb;
  std::size_t n = boost::asio::read_until(ws, sb, '\n');
  net::streambuf::const_buffers_type bufs = sb.data();
  std::string line(
      net::buffers_begin(bufs),
      net::buffers_begin(bufs) + n -1);
  sb.consume(n);
  return line;
}

SCENARIO("Testing network component") {
  GIVEN("Websocket is connected to the service") {
    init_logging(boost::log::trivial::warning);

    Options options {
      .address = "0.0.0.0",
      .port = "8080",
      .pathToMonitor = "",
      .pathsToExclude = {},
      .logSeverity = boost::log::trivial::severity_level::info
    };

    BOOST_LOG_TRIVIAL(info) << "starting the service\n";

    std::thread([&options](){
      runService(options, TestFactory{options});
    }).detach();

    net::io_context ioc;
    websocket::stream<tcp::socket> ws{ioc};
    tcp::resolver resolver{ioc};

    connectToServer(resolver, ws);

    WHEN("client is subscribed") {
      subscribe(ws, IN_CLOSE_WRITE | IN_MOVED_TO);
      // An unfortunate sleep, we need to make sure
      // our subscription has been processed by the service
      sleep_for(milliseconds(10));
      { // we have to make sure the mutex is released!
        std::unique_lock<std::mutex> lck(g_mtx);

        BOOST_LOG_TRIVIAL(info) << "Notifying TestMessageProvide it can send its test messages";
        g_ready = true;
        g_cv.notify_all();
      }

      THEN("We receive the expected message from the service") {
        BOOST_LOG_TRIVIAL(info) << "Reading a line from the websocket...";
        std::string receivedMessage = readLine(ws);
        REQUIRE(receivedMessage == "Message 42"); // note the absence of the terminating EOL
        REQUIRE(isFilteredOut);
      }
    }

    BOOST_LOG_TRIVIAL(info) << "closing the websocket\n";
    ws.close(websocket::close_code::normal);

    //This mostly works...
    //BOOST_LOG_TRIVIAL(info) << "Sending SIGNINT signal\n";
    //kill(getpid(), SIGINT);
  }
}

