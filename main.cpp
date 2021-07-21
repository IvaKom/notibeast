#include <boost/log/trivial.hpp>
#include <boost/optional/optional_io.hpp>

#include <sstream>
#include <algorithm>

#include "glue/options.h"
#include "beast/service.hpp"
#include "logging.h"
#include "cl_parser.h"
#include "recursive_i_notify_factory.h"


int main(int argc, char** argv) {
  initLogging();

  std::ostringstream sstr;
  std::for_each(argv, argv + argc,
      [&sstr, sep=""](const char *arg) mutable {
        sstr << sep << arg; sep = " ";
      }
  );
  BOOST_LOG_TRIVIAL(info) << sstr.str();

  try {
    auto options = parseCommandLine(argc, argv);
    if (!options) { // called with '--help'
      return EXIT_FAILURE;
    }
    BOOST_LOG_TRIVIAL(info) << "Run service with parameters: '" << options << "'\n";
    setSeverityLevel(options->logSeverity);
    runService(*options, RecursiveINotifyFactory{*options});
  }
  catch(std::exception& e) {
      BOOST_LOG_TRIVIAL(error) << "error: " << e.what();
      return EXIT_FAILURE;
  }
  catch(...) {
      BOOST_LOG_TRIVIAL(error) << "Unknown exception";
      return EXIT_FAILURE;
  }

  // (If we get here, it means we got a SIGINT or SIGTERM)

  return EXIT_SUCCESS;
}

