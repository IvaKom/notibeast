#ifndef LOGGING_H
#define LOGGING_H

#include <boost/log/trivial.hpp>

void initLogging();
void setSeverityLevel(boost::log::trivial::severity_level level);

#endif
