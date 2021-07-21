#include "i_notify.h"

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <sys/inotify.h>
#include "helper.h"
#include <mutex>


using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds

SCENARIO("Testing NofityingFS, current dir only") {
  GIVEN("INotify is created for a tmp dir") {
    init_logging();
    auto ph = createTempDir("test_notify_fs_");

    REQUIRE(fs::exists(ph));
    //std::cout << "Tmp directory " << ph << " is created\n";
    std::vector<NotifyEvent> events;
    std::mutex mtx;
    auto callback=[&events, &mtx](NotifyEvent event){
      std::lock_guard<std::mutex> lg(mtx);
      events.push_back(event);
    };

    WHEN("File is accessed") {
      INotify nfs(callback);
      {std::ofstream(ph/"foo");}
      nfs.monitorPath(ph);

      std::ifstream{ph/"foo"};

      sleep_for(milliseconds(1));
      THEN("We receive OPEN, CLOSE_NOWRITE notifications") {
        // strange thing there is no IN_ACCESS notification,
        // even when we read the file
        std::lock_guard<std::mutex> lg(mtx);

        REQUIRE(events.size()==2);

        REQUIRE(events[0].mask == (IN_OPEN));
        CHECK(events[0].cookie == 0);
        CHECK(events[0].name == "foo");

        REQUIRE(events[1].mask == (IN_CLOSE_NOWRITE));
        CHECK(events[1].cookie == 0);
        CHECK(events[0].name == "foo");
      }
    }

    WHEN("Directory is accessed") {
      INotify nfs(callback);
      nfs.monitorPath(ph);
      auto dit = fs::directory_iterator(ph);

      sleep_for(milliseconds(1));
      THEN("We receive OPEN, ACCESS, CLOSE_NOWRITE notifications") {
        std::lock_guard<std::mutex> lg(mtx);

        CHECK(events.size()==3);

        REQUIRE(events[0].mask == (IN_OPEN|IN_ISDIR));
        CHECK(events[0].cookie == 0);
        CHECK(events[0].name == "");

        REQUIRE(events[1].mask == (IN_ACCESS|IN_ISDIR));
        CHECK(events[1].cookie == 0);
        CHECK(events[1].name == "");

        REQUIRE(events[2].mask == (IN_CLOSE_NOWRITE|IN_ISDIR));
        CHECK(events[2].cookie == 0);
        CHECK(events[2].name == "");
      }
    }

    WHEN("File is created") {
      INotify nfs(callback);
      nfs.monitorPath(ph);
      std::ofstream(ph/"foo");

      sleep_for(milliseconds(1));
      THEN("We receive CREATE, OPEN, CLOSE_WRITE notifications") {
        CHECK(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);

        CHECK(events.size()==3);

        REQUIRE(events[0].mask == IN_CREATE);
        CHECK(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");

        REQUIRE(events[1].mask == IN_OPEN);
        CHECK(events[1].cookie == 0);
        REQUIRE(events[1].name == "foo");

        REQUIRE(events[2].mask == IN_CLOSE_WRITE);
        CHECK(events[2].cookie == 0);
        REQUIRE(events[2].name == "foo");
      }
      fs::remove(ph/"foo");
    }

    WHEN("File's permissions changed") {
      INotify nfs(callback);
      std::ofstream(ph/"foo");
      nfs.monitorPath(ph);

      fs::permissions(ph/"foo", fs::perms::owner_all);

      sleep_for(milliseconds(1));
      THEN("We receive ATTRIB notification") {
        CHECK(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);

        CHECK(events.size()==1);

        REQUIRE(events[0].mask == IN_ATTRIB);
        CHECK(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");
      }
      fs::remove(ph/"foo");
    }

    WHEN("File is deleted") {
      std::ofstream(ph/"foo");
      CHECK(fs::exists(ph / "foo"));
      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::remove(ph/"foo");
      sleep_for(milliseconds(1));
      THEN("We receive DELETE notification") {
        CHECK_FALSE(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==1);

        REQUIRE(events[0].mask == IN_DELETE);
        CHECK(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");
      }
    }

    WHEN("Directory is created") {
      INotify nfs(callback);
      nfs.monitorPath(ph);
      fs::create_directory(ph/"foo");

      sleep_for(milliseconds(1));
      THEN("We receive (IN_CREATE | IN_ISIDR) notification") {
        CHECK(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);

        CHECK(events.size()==1);

        REQUIRE(events[0].mask == (IN_CREATE | IN_ISDIR));
        CHECK(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");
      }
      fs::remove(ph/"foo");
    }

    WHEN("Directory is deleted") {
      fs::create_directory(ph/"foo");
      CHECK(fs::exists(ph / "foo"));
      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::remove(ph/"foo");
      sleep_for(milliseconds(1));
      THEN("We receive (IN_DELETE|IN_ISDIR) notification") {
        CHECK_FALSE(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==1);

        REQUIRE(events[0].mask == (IN_DELETE | IN_ISDIR));
        CHECK(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");
      }
    }

    WHEN("Self directory is deleted") {
      fs::create_directory(ph/"foo");
      CHECK(fs::exists(ph / "foo"));
      INotify nfs(callback);
      nfs.monitorPath(ph/"foo");

      fs::remove(ph/"foo");
      sleep_for(milliseconds(1));
      THEN("We receive IN_DELETE_SELF, IN_IGNORED notifications") {
        CHECK_FALSE(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==2);

        REQUIRE(events[0].mask == (IN_DELETE_SELF));
        CHECK(events[0].cookie == 0);
        REQUIRE(events[0].name == "");

        REQUIRE(events[1].mask == (IN_IGNORED));
        CHECK(events[1].cookie == 0);
        REQUIRE(events[1].name == "");
      }
    }

    WHEN("File is moved inside") {
      std::ofstream(ph/"foo");
      CHECK(fs::exists(ph / "foo"));
      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::rename(ph/"foo", ph/"bar");
      sleep_for(milliseconds(1));
      THEN("We receive IN_MOVED_FROM, IN_MOVED_TO notifications and have a cookie") {
        CHECK_FALSE(fs::exists(ph / "foo"));
        CHECK(fs::exists(ph / "bar"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==2);

        REQUIRE(events[0].mask == IN_MOVED_FROM);
        REQUIRE_FALSE(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");

        REQUIRE(events[1].mask == IN_MOVED_TO);
        REQUIRE(events[1].cookie == events[0].cookie);
        REQUIRE(events[1].name == "bar");
      }
    }

    WHEN("Directory is moved inside") {
      fs::create_directory(ph/"foo");
      CHECK(fs::exists(ph / "foo"));
      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::rename(ph/"foo", ph/"bar");
      sleep_for(milliseconds(1));
      THEN("We receive (IN_MOVED_FROM|IN_ISDIR), "
           "(IN_MOVED_TO|IN_ISDIR) notifications and have a cookie") {
        CHECK_FALSE(fs::exists(ph / "foo"));
        CHECK(fs::exists(ph / "bar"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==2);

        REQUIRE(events[0].mask == (IN_MOVED_FROM | IN_ISDIR));
        REQUIRE_FALSE(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");

        REQUIRE(events[1].mask == (IN_MOVED_TO | IN_ISDIR));
        REQUIRE(events[1].cookie == events[0].cookie);
        REQUIRE(events[1].name == "bar");
      }
    }

    WHEN("File is moved outside") {
      std::ofstream(ph/"foo");
      CHECK(fs::exists(ph / "foo"));

      auto ph1 = createTempDir("test_notify_fs_");
      REQUIRE(fs::exists(ph1));

      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::rename(ph/"foo", ph1/"foo");
      sleep_for(milliseconds(1));
      THEN("We receive IN_MOVED_FROM notifications and have a cookie") {
        CHECK_FALSE(fs::exists(ph / "foo"));
        CHECK(fs::exists(ph1 / "foo"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==1);

        REQUIRE(events[0].mask == IN_MOVED_FROM);
        REQUIRE_FALSE(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");
      }
      fs::remove_all(ph1);
      REQUIRE_FALSE(fs::exists(ph1));
    }

    WHEN("File is moved from outside") {
      auto ph1 = createTempDir("test_notify_fs_");
      REQUIRE(fs::exists(ph1));

      std::ofstream(ph1/"foo");
      CHECK(fs::exists(ph1 / "foo"));

      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::rename(ph1/"foo", ph/"foo");
      sleep_for(milliseconds(1));
      THEN("We receive IN_MOVED_TO notifications and have a cookie") {
        CHECK_FALSE(fs::exists(ph1 / "foo"));
        CHECK(fs::exists(ph / "foo"));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==1);

        REQUIRE(events[0].mask == IN_MOVED_TO);
        REQUIRE_FALSE(events[0].cookie == 0);
        REQUIRE(events[0].name == "foo");
      }
      fs::remove_all(ph1);
      REQUIRE_FALSE(fs::exists(ph1));
    }

    WHEN("Self is moved") {
      INotify nfs(callback);
      nfs.monitorPath(ph);

      auto newPath=fs::path(std::string(ph)+"foo");
      fs::rename(ph, newPath) ;
      sleep_for(milliseconds(1));
      THEN("We receive IN_MOVE_SELF notifications") {
        CHECK_FALSE(fs::exists(ph));
        CHECK(fs::exists(newPath));
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==1);

        REQUIRE(events[0].mask == IN_MOVE_SELF);
        REQUIRE(events[0].cookie == 0);
        REQUIRE(events[0].name == "");
      }
      {
         std::lock_guard<std::mutex> lg(mtx);
         events.clear();
      }
      std::ofstream{newPath/"bar"};
      sleep_for(milliseconds(1));
      THEN("And continue notifications") {
        std::lock_guard<std::mutex> lg(mtx);
        REQUIRE(events.size()==3);
      }
      fs::remove_all(newPath);
      REQUIRE_FALSE(fs::exists(newPath));
    }

    WHEN("Self is deleted") {
      INotify nfs(callback);
      nfs.monitorPath(ph);

      fs::remove_all(ph) ;
      REQUIRE_FALSE(fs::exists(ph));

      sleep_for(milliseconds(1));
      THEN("We receive IN_DELETE_SELF and Co notifications") {
        std::lock_guard<std::mutex> lg(mtx);
        CHECK(events.size()==5);

        REQUIRE(events[0].mask == (IN_OPEN|IN_ISDIR));
        REQUIRE(events[0].cookie == 0);
        REQUIRE(events[0].name == "");

        REQUIRE(events[1].mask == (IN_ACCESS|IN_ISDIR));
        REQUIRE(events[1].cookie == 0);
        REQUIRE(events[1].name == "");

        REQUIRE(events[2].mask == (IN_CLOSE_NOWRITE|IN_ISDIR));
        REQUIRE(events[2].cookie == 0);
        REQUIRE(events[2].name == "");

        REQUIRE(events[3].mask == IN_DELETE_SELF);
        REQUIRE(events[3].cookie == 0);
        REQUIRE(events[3].name == "");

        REQUIRE(events[4].mask == IN_IGNORED);
        REQUIRE(events[4].cookie == 0);
        REQUIRE(events[4].name == "");
      }
    }

    /*WHEN("Attempting to monitor out-of-tree") {
      auto ph1 = createTempDir("test_notify_fs_");
      REQUIRE(fs::exists(ph1));

      INotify nfs(callback);
      REQUIRE(nfs.monitorPath(ph1)==0);

    } */

    // cleaning up
    //std::cout<<"removing the directory\n";
    fs::remove_all(ph);
    REQUIRE_FALSE(fs::exists(ph));
  }

}

