#ifndef OPTIONS_H
#define OPTIONS_H

#include <boost/log/trivial.hpp>
#include <ostream>

struct Options {
  std::string address;
  std::string port;
  std::string pathToMonitor;
  std::vector<std::string> pathsToExclude;
  boost::log::trivial::severity_level logSeverity;
};

namespace std {
// looks like placing it inside std is the only way
// to have BOOST_LOG recognize it
std::ostream& operator <<(std::ostream& s, const Options &o);
}

#endif
