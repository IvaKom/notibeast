#include "recursive_i_notify.h"

#include "recursive_notify_event.h"
#include "i_notify.h"
#include "i_notify_helper.h"

#include <iostream>
#include <sys/inotify.h>
#include <vector>
#include <cassert>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <boost/log/trivial.hpp>

namespace std {
  template <>
  struct hash<fs::path> {
    std::size_t operator()(const fs::path &path) const {
      return hash_value(path);
    }
  };
}

namespace {
RecursiveNotifyEvent makeRecursive(NotifyEvent const &ne,
                                   std::string const &path) {
  return {.mask = ne.mask,
          .cookie = ne.cookie,
          .path = path,
          .name = ne.name};
}

// fs::path dies if the path in question doesn't exist anymore
// fall back to trimming the string representation of the path in such a case
fs::path safe_relative_path(const fs::path &p, const fs::path &base) {
  std::error_code ec;
  auto relPath = fs::relative(p, base, ec);
  if (!ec) {
    return relPath;
  }

  BOOST_LOG_TRIVIAL(debug) << "Failed to find a relative path"
    << ". Error is: " << ec
    << ". base path " << base << " exists? " << fs::exists(base)
    << ". p path " << p << " exists? " << fs::exists(p);
  auto pathStr = p.string();
  auto baseStr = base.string();
  if (size_t pos = pathStr.find(baseStr);
      !fs::exists(p) && pos == 0)
  {
    std::string_view pathView = pathStr;
    pathView.remove_prefix(baseStr.length() + 1);
    relPath = pathView;
    BOOST_LOG_TRIVIAL(debug) << "constructed relPath: " << relPath;

    return relPath;
  } else {
    std::stringstream sstr;
    sstr << "Failed to construct a relative path for path " << p
      << " and base " << base
      << ". Error: " << ec;
    throw std::runtime_error(sstr.str());
  }
}

} //namespace

using namespace std;

class RecursiveINotify::RecursiveINotifyImpl {
public:
  explicit RecursiveINotifyImpl(std::function<void(RecursiveNotifyEvent)> rfn,
                                fs::path const &rootPath,
                                std::vector<std::string> pathsToSkip):
    rfn{rfn},
    notifier{std::make_unique<INotify>(
       [this, rootPath](NotifyEvent const &ne) {
         try {
           handleEvent(rootPath, ne);
         } catch (std::exception &ec) {
           BOOST_LOG_TRIVIAL(warning) << "Exception occured while processing event " << ne
             << ". Error is: " << ec.what();
         }
       }
    )},
    pathsToSkip(std::move(pathsToSkip))
  {
    monitorDirRecursively(rootPath);
  }

  ~RecursiveINotifyImpl() {
  }

  RecursiveINotifyImpl(RecursiveINotifyImpl const &) = delete;
  RecursiveINotifyImpl& operator=(RecursiveINotifyImpl const&) = delete;

private:
  std::function<void(RecursiveNotifyEvent)> rfn;
  std::unique_ptr<INotify> notifier;
  std::unordered_map<fs::path, int> rPathMap;
  std::unordered_map<int, fs::path> pathMap;
  std::unordered_set<fs::path> ignoredPaths;
  std::unordered_set<fs::path> beingUnmountedPaths;
  std::vector<std::string> pathsToSkip;

private:
  void monitorDirRecursively(const fs::path &path) {
    BOOST_LOG_TRIVIAL(info) << "Indexing monitoring directory " << path;
    vector<fs::path> children;
    children.push_back(path);
    for(auto &item: fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)){
      BOOST_LOG_TRIVIAL(trace) << "Visiting '" << item.path() << "' directory";
      if (item.is_directory()) {
        if (std::any_of(pathsToSkip.begin(), pathsToSkip.end(), [&item](std::string const &pts) {
          return item.path().string().find(pts) != std::string::npos;
        })) {
          continue;
        }
        children.push_back(item);
      }
    }
    for(auto &dp: children) {
      int wd = notifier->monitorPath(dp);
      BOOST_LOG_TRIVIAL(trace) << "Monitoring path " << dp << " with wd " << wd;

      pathMap[wd]=dp;
      rPathMap[dp]=wd;
    }
    BOOST_LOG_TRIVIAL(info) << "Start monitoring, " << pathMap.size() << " subdirectories indexed";
  }

  void handleEvent(const fs::path &rootPath, NotifyEvent const &ne) {
    BOOST_LOG_TRIVIAL(debug) << "Event for wd " << ne.wd
      << ", name: " << ne.name
      << ", mask: " << strMask(ne.mask);

    if (ne.mask == IN_Q_OVERFLOW) {
      //no idea how to test it, shooting into the air
      assert(ne.wd == -1);
      publishEvent(ne, "");
      return;
    }

    auto pathIt = pathMap.find(ne.wd);
    if (pathIt == end(pathMap)) {
      BOOST_LOG_TRIVIAL(debug) << "ignore event " << ne;
      if (ne.mask != IN_IGNORED) { //leftovers of removed or moved-from nested dirs
        BOOST_LOG_TRIVIAL(warning) << "ignore event " << ne;
      }
      return;
    }

    auto relPath = safe_relative_path(pathIt->second, rootPath);
    BOOST_LOG_TRIVIAL(debug) << "relPath: " << relPath << ", name: '" << ne.name << "'";

    if (ne.mask == (IN_MOVED_FROM | IN_ISDIR)) {
      enterMovedFrom(pathIt->second/ne.name);
    }

    if (ne.mask == IN_UNMOUNT) {
      beingUnmountedPaths.insert(relPath);
      if (relPath != ".") {
        BOOST_LOG_TRIVIAL(debug) << "ignore IN_UNMOUNT for path " << pathIt->second
          << ", name: '" << ne.name << "', mask: " << strMask(ne.mask);
        return;
      }
    }

    if ((ne.mask == (IN_CREATE | IN_ISDIR) || ne.mask == (IN_MOVED_TO | IN_ISDIR))) {
      monitorDirRecursively(pathIt->second/ne.name);
    }

    if (ne.mask == IN_IGNORED) {
      auto bup = beingUnmountedPaths.find(relPath);
      if (bup != beingUnmountedPaths.end()) {
        beingUnmountedPaths.erase(bup);
        if (relPath == ".") {
          publishEvent(ne, relPath);
        } else {
          BOOST_LOG_TRIVIAL(debug) << "ignore IN_IGNORED for path " << pathIt->second
            << ", name: '" << ne.name << "', mask: " << strMask(ne.mask);
        }
        return;
      }
      completeMovedFrom(pathIt);
    } else if (shallIgnorePath(pathIt->second)) {
      BOOST_LOG_TRIVIAL(debug) << "ignore path " << pathIt->second
        << ", name: '" << ne.name << "', mask: " << strMask(ne.mask);
      return;
    }

    publishEvent(ne, relPath);
  }

  void enterMovedFrom(const fs::path &path) {
    auto itWd = rPathMap.find(path);
    assert(itWd!=rPathMap.end());
    BOOST_LOG_TRIVIAL(debug) << "Removing watch for wd " << itWd->second;
    ignoredPaths.insert(itWd->first);
    notifier->removeWatch(itWd->second);
  }

  void completeMovedFrom(unordered_map<int, fs::path>::iterator pathIt) {
    BOOST_LOG_TRIVIAL(debug) << "Ignored event landed, cleaning stuff";
    auto erased = ignoredPaths.erase(pathIt->second);
    //NOTE: consider using C++20 erase_if
    for (auto mPathIt = begin(pathMap); mPathIt != end(pathMap); ) {
      auto relPath=fs::relative(mPathIt->second, pathIt->second);
      BOOST_LOG_TRIVIAL(trace) << relPath;
      if (*relPath.begin() != ".." &&  pathIt != mPathIt) {
        BOOST_LOG_TRIVIAL(debug) << mPathIt->second << " needs cleaning";
        notifier->removeWatch(mPathIt->first);
        erased = rPathMap.erase(mPathIt->second);
        assert(erased == 1);
        mPathIt = pathMap.erase(mPathIt);
      } else {
        ++mPathIt;
      }
    }

    erased = rPathMap.erase(pathIt->second);
    assert(erased == 1);
    pathMap.erase(pathIt);
    BOOST_LOG_TRIVIAL(debug) << "Ignored event completed";
  }

  void publishEvent(NotifyEvent const &ne, const fs::path &relPath) const {
    RecursiveNotifyEvent rne = makeRecursive(ne, relPath);

    BOOST_LOG_TRIVIAL(debug) << "Publish event for " << relPath
      << ", name: '" << rne.name
      << "', mask: " << strMask(rne.mask);
    rfn(rne);
  }

  bool shallIgnorePath(const fs::path &path) const {
    return std::any_of(begin(ignoredPaths), end(ignoredPaths), [&path](const fs::path &ip) {
      auto relToIp=fs::relative(path, ip);
      return *relToIp.begin() != "..";
    });
  }

}; // RecursiveINotifyImpl

RecursiveINotify::RecursiveINotify(std::function<void(RecursiveNotifyEvent)> rfn,
                 fs::path const &rootPath,
                 std::vector<std::string> pathsToSkip):
  pImpl(make_unique<RecursiveINotifyImpl>(rfn, rootPath, std::move(pathsToSkip)))
{
  BOOST_LOG_TRIVIAL(debug) <<"RecursiveINotify::ctor()";
}

RecursiveINotify::~RecursiveINotify() {
  BOOST_LOG_TRIVIAL(debug) <<"RecursiveINotify::dtor()";
}

void RecursiveINotify::logFiltered(std::string const &ss, int filteringMask) const {
  std::string_view sv = ss;
  if (!sv.empty() && *sv.rbegin() == '\n') {
    sv.remove_suffix(1);
  }
  BOOST_LOG_TRIVIAL(debug) << "Message '" << sv << "' is filtered out by mask " << strMask(filteringMask) << "";
}

void RecursiveINotify::logSubscribing(int mask) const {
  BOOST_LOG_TRIVIAL(info) << "Subscribing for mask " << strMask(mask);
}
