#include "recursive_i_notify.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <sys/inotify.h>
#include "helper.h"
#include <mutex>
#include <sstream>

#include "i_notify_helper.h"
#include "recursive_notify_event.h"

using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds
using Catch::Matchers::VectorContains;

namespace std {
  std::ostream& operator<< (std::ostream &out, const RecursiveNotifyEvent &rne) {
    out << "{mask: \"" << strMask(rne.mask)
        << "\", cookie: " << rne.cookie
	<< ", path: \"" << rne.path
	<< "\", name: \"" << rne.name
	<< "\"}";
    return out;
  }
}

bool operator == (RecursiveNotifyEvent const &lhs, RecursiveNotifyEvent const &rhs) {
  return lhs.mask == rhs.mask
      && lhs.cookie == rhs.cookie
      && lhs.name == rhs.name
      && lhs.path == rhs.path;
}

SCENARIO("Testing NofityingFS, nested structures") {
  GIVEN("RecursiveINotify is created for a tmp dir") {
    init_logging();
    auto ph = createTempDir("test_notify_fs_");
    REQUIRE(fs::exists(ph));
    //std::cout << "Tmp directory " << ph << " is created\n";
    std::vector<RecursiveNotifyEvent> events;
    std::mutex mtx;
    auto callback=[&events, &mtx](RecursiveNotifyEvent event){
      //std::cout << "callback is called\n";
      std::lock_guard<std::mutex> lg(mtx);
      events.push_back(event);
    };

    WHEN("Nested file in a nested directory is accessed") {
      auto nestedPath=ph/"nested.d";
      fs::create_directory(nestedPath);
      {std::ofstream(nestedPath/"foo");}

      RecursiveINotify nfs(callback, ph);
      std::ifstream{nestedPath/"foo"};

      sleep_for(milliseconds(2));
      THEN("We receive OPEN, CLOSE_NOWRITE notifications") {
        // strange thing there is no IN_ACCESS notification,
        // even when we read the file
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.size()==2);

        CHECK(events[0] == RecursiveNotifyEvent{IN_OPEN, 0, "nested.d", "foo"});
        CHECK(events[1] == RecursiveNotifyEvent{IN_CLOSE_NOWRITE, 0, "nested.d", "foo"});
      }
      //std::cout<<"Finishing test case\n";
    }

    WHEN("Nested dir is moved out") {
      auto phOut = createTempDir("test_notify_fs_");
      REQUIRE(fs::exists(phOut));

      auto nestedPath=ph/"nested.d";
      fs::create_directory(nestedPath);
      auto nestedNestedPath=nestedPath/"nestedNested.d";
      fs::create_directory(nestedNestedPath);
      {std::ofstream(nestedNestedPath/"foo");}
      RecursiveINotify nfs(callback, ph);
      rename(nestedPath, phOut/"nested.d");
      std::ofstream{phOut/"nested.d"/"foo"};
      std::ofstream{ph/"bar"};

      sleep_for(milliseconds(10));
      THEN("Right things happen") {
        // strange thing there is no IN_ACCESS notification,
        // even when we read the file
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.size()>=5);
        CHECK(events.size()==5);

        CHECK(events[0].mask == (IN_MOVED_FROM | IN_ISDIR));
        CHECK(events[0].cookie != 0);
        CHECK(events[0].name == "nested.d");
        CHECK(events[0].path == ".");

        CHECK_THAT(events, VectorContains(RecursiveNotifyEvent{IN_IGNORED, 0, "nested.d", ""}));
        CHECK_THAT(events, VectorContains(RecursiveNotifyEvent{IN_CREATE, 0, ".", "bar"}));
        CHECK_THAT(events, VectorContains(RecursiveNotifyEvent{IN_OPEN, 0, ".", "bar"}));
        CHECK_THAT(events, VectorContains(RecursiveNotifyEvent{IN_CLOSE_WRITE, 0, ".", "bar"}));
      }
      //std::cout<<"Finishing test case\n";
      fs::remove_all(phOut);
      //std::cout<<"Test case finished\n";
    }

    WHEN("Nested dir is moved in") {
      auto phOut = createTempDir("test_notify_fs_");
      REQUIRE(fs::exists(phOut));

      auto nestedPath=phOut/"nested.d";
      fs::create_directory(nestedPath);
      auto nestedNestedPath=nestedPath/"nestedNested.d";
      fs::create_directory(nestedNestedPath);

      RecursiveINotify nfs(callback, ph);

      rename(nestedPath, ph/"nested.d");
      // we can't catch subsequent nested events without yielding
      sleep_for(milliseconds(10));
      //yield();
      {std::ofstream(ph/"nested.d"/"nestedNested.d"/"foo");}

      sleep_for(milliseconds(10));
      THEN("Right things happen") {
        // strange thing there is no IN_ACCESS notification,
        // even when we read the file
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.size()>=7);
        CHECK(events.size()==7);

        CHECK(events[0].mask == (IN_MOVED_TO | IN_ISDIR));
        CHECK(events[0].cookie != 0);
        CHECK(events[0].name == "nested.d");
        CHECK(events[0].path == ".");

        CHECK(events[1] == RecursiveNotifyEvent{IN_OPEN | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[2] == RecursiveNotifyEvent{IN_ACCESS | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[3] == RecursiveNotifyEvent{IN_CLOSE_NOWRITE | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[4] == RecursiveNotifyEvent{IN_CREATE, 0, "nested.d/nestedNested.d", "foo"});
        CHECK(events[5] == RecursiveNotifyEvent{IN_OPEN, 0, "nested.d/nestedNested.d", "foo"});
        CHECK(events[6] == RecursiveNotifyEvent{IN_CLOSE_WRITE, 0, "nested.d/nestedNested.d", "foo"});
      }
      fs::remove_all(phOut);
    }

    WHEN("Nested dir is created") {

      RecursiveINotify nfs(callback, ph);

      auto nestedPath=ph/"nested.d";
      fs::create_directory(nestedPath);

      // we can't catch subsequent nested events without yielding
      sleep_for(milliseconds(10));
      //yield();
      {std::ofstream(nestedPath/"foo");}

      sleep_for(milliseconds(10));
      THEN("Right things happen") {
        // strange thing there is no IN_ACCESS notification,
        // even when we read the file
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.size()>=7);
        CHECK(events.size()==7);

        CHECK(events[0] == RecursiveNotifyEvent{IN_CREATE | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[1] == RecursiveNotifyEvent{IN_OPEN | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[2] == RecursiveNotifyEvent{IN_ACCESS | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[3] == RecursiveNotifyEvent{IN_CLOSE_NOWRITE | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[4] == RecursiveNotifyEvent{IN_CREATE, 0, "nested.d", "foo"});
        CHECK(events[5] == RecursiveNotifyEvent{IN_OPEN, 0, "nested.d", "foo"});
        CHECK(events[6] == RecursiveNotifyEvent{IN_CLOSE_WRITE, 0, "nested.d", "foo"});
      }
    }

    WHEN("Nested dir is deleted") {
      auto nestedPath=ph/"nested.d";
      fs::create_directory(nestedPath);
      {std::ofstream(nestedPath/"foo");}

      RecursiveINotify nfs(callback, ph);
      fs::remove_all(nestedPath);
      sleep_for(milliseconds(10));

      THEN("Right things happen") {
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.size()>=12);
        CHECK(events.size()==12);

        CHECK(events[0]  == RecursiveNotifyEvent{IN_OPEN | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[1]  == RecursiveNotifyEvent{IN_OPEN | IN_ISDIR, 0, "nested.d", ""});
        CHECK(events[2]  == RecursiveNotifyEvent{IN_ACCESS | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[3]  == RecursiveNotifyEvent{IN_ACCESS | IN_ISDIR, 0, "nested.d", ""});
        CHECK(events[4]  == RecursiveNotifyEvent{IN_DELETE, 0, "nested.d", "foo"});
        CHECK(events[5]  == RecursiveNotifyEvent{IN_ACCESS | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[6]  == RecursiveNotifyEvent{IN_ACCESS | IN_ISDIR, 0, "nested.d", ""});
        CHECK(events[7]  == RecursiveNotifyEvent{IN_CLOSE_NOWRITE | IN_ISDIR, 0, ".", "nested.d"});
        CHECK(events[8]  == RecursiveNotifyEvent{IN_CLOSE_NOWRITE | IN_ISDIR, 0, "nested.d", ""});
        REQUIRE_THAT(events, VectorContains(RecursiveNotifyEvent{IN_DELETE_SELF, 0, "nested.d", ""}));
        REQUIRE_THAT(events, VectorContains(RecursiveNotifyEvent{IN_IGNORED, 0, "nested.d", ""}));
        REQUIRE_THAT(events, VectorContains(RecursiveNotifyEvent{IN_DELETE | IN_ISDIR, 0, ".", "nested.d"}));
     }
    }

    WHEN("File created in ignored dir") {
      auto nestedPath=ph/"ignore.d";
      fs::create_directory(nestedPath);

      RecursiveINotify nfs(callback, ph, {"ignore.d"});
      {std::ofstream(nestedPath/"foo");}
      sleep_for(milliseconds(10));

      THEN("No events published") {
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.empty());
      }
    }

    // cleaning up
    //std::cout<<"removing the directory\n";
    fs::remove_all(ph);
    REQUIRE_FALSE(fs::exists(ph));
  }

}
