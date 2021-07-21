#include "cl_parser.h"

#include <boost/program_options.hpp>
#include <boost/optional/optional_io.hpp>

namespace po = boost::program_options;

//Unset Options indicate --help being requested
boost::optional<Options> parseCommandLine(int argc, char **argv) {
  Options res;
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
      ("address,a", po::value(&res.address)->default_value("0.0.0.0"), "TCP address of the binding interface.")
      ("port,p", po::value(&res.port)->default_value("8080"), "Which port to listen to.")
      ("monitor_path,m", po::value(&res.pathToMonitor)->required(), "Path to the directory to monitor")
      ("path_to_exclude,x", po::value(&res.pathsToExclude), "Path(s) to exclude from monitoring. Doesn't have to be a full path.")
      ("log_severity,l", po::value(&res.logSeverity)->default_value(boost::log::trivial::info), "log level to output");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv)
      .options(desc).run(), vm);

  if (vm.count("help")) {
      BOOST_LOG_TRIVIAL(info) << desc
        << "e.g.:\n./notibeast -a 0.0.0.0 -p 8080 -m /tmp  -x '@eaDir' -x '#recycle' -l debug";
      return {};
  }

  po::notify(vm);

  return res;
}
