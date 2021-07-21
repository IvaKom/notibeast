#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <string>
#include <filesystem>
#include <boost/log/trivial.hpp>

std::filesystem::path
createTempDir(std::string_view prefix);

void init_logging(boost::log::trivial::severity_level severity = boost::log::trivial::warning);
#endif
