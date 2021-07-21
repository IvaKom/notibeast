#include "options.h"

namespace std {

std::ostream& operator <<(std::ostream& s, const Options &o) {
  s << "address: " << o.address
    << ", port: " << o.port
    << ", pathToMonitor: " << o.pathToMonitor
    << ", pathsToExclude: [";
  const char *sep = "";
  for(auto &p: o.pathsToExclude) {
    s << sep; sep = ", ";
    s << p;
  }
  s << "], logSeverity: " << o.logSeverity;
  return s;
}

}

