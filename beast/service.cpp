#include "service.hpp"

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

#include "listener.hpp"
#include "shared_state.hpp"

namespace net = boost::asio;                    // from <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

void runService(const Options &options, const MessageProviderFactory &mpFactory) {
  auto address = net::ip::make_address(options.address);
  auto port = static_cast<unsigned short>(std::stoi(options.port));

  // The io_context is required for all I/O
  net::io_context ioc;

  // Create and launch a listening port
  boost::make_shared<listener>(
    ioc,
    tcp::endpoint{address, port},
    boost::make_shared<shared_state>(mpFactory)
  )->run();

  // Capture SIGINT and SIGTERM to perform a clean shutdown
  net::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait(
    [&ioc](boost::system::error_code const&, int) {
      BOOST_LOG_TRIVIAL(info) << "Caught SIGINT or SIGTERM";
      // Stop the io_context. This will cause run()
      // to return immediately, eventually destroying the
      // io_context and any remaining handlers in it.
      ioc.stop();
    });

  ioc.run();
}
