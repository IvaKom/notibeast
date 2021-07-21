#include "logging.h"

#include <iostream> //for std::clog
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;

void initLogging() {
  // TODO: include function name, see
  // http://en.cppreference.com/w/cpp/utility/source_location/function_name
  logging::add_common_attributes();
  logging::add_console_log (
    std::clog,
    logging::keywords::format = "[%ThreadID%] [%Severity%]: %Message%"
  );

  logging::core::get()->set_filter (
    logging::trivial::severity >= logging::trivial::info
  );
}

void setSeverityLevel(logging::trivial::severity_level level) {
  logging::core::get()->set_filter (
    logging::trivial::severity >= level
  );
}
