#include "helper.h"
#include <filesystem>
#include <random>
#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace fs = std::filesystem;
namespace logging = boost::log;

fs::path createTempDir(std::string_view prefix) {
  std::random_device dev;
  std::mt19937 prng(dev());
  std::uniform_int_distribution<uint64_t> rand(0);
  std::stringstream ss;
  ss << prefix << std::hex << rand(prng);
  fs::path ph = fs::temp_directory_path() / ss.str();
  std::error_code ec;
  while(!create_directories(ph, ec) && ec) {
    BOOST_LOG_TRIVIAL(info) << "Couldn't create temporary directory " << ph
      <<". Error: " << ec;
  }
  return ph;
}
 
void init_logging(boost::log::trivial::severity_level severity) {
  // TODO: include function name, see
  // http://en.cppreference.com/w/cpp/utility/source_location/function_name
  logging::add_common_attributes();
  logging::add_console_log (
    std::clog,
    logging::keywords::format = "[%ThreadID%] [%Severity%]: %Message%"
  );

  logging::core::get()->set_filter (
    logging::trivial::severity >= severity
  );
}
