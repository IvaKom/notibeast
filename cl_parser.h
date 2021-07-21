#ifndef CL_PARSER_HPP
#define CL_PARSER_HPP

#include <boost/optional/optional.hpp>
#include "glue/options.h"

boost::optional<Options> parseCommandLine(int argc, char **argv);

#endif
