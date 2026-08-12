// One-off edge-case verification (run: bazel test //:db_edge_test)
#include "src/storage/db.h"
#include <cstdlib>
#include <string>
#include "src/utils/test_harness.h"

namespace focuskv {

static void CleanDir(const std::string& path) {
  system(("rm -rf " + path).c_str());
}

TEST(DBEdgeTest, OverwriteAcrossFlushAndReopen) {
  std::string path = "/tmp/focuskv_edge_overwrite";
  CleanDir(path);

  Options opts;
  opts.create_if_missing = true;
  opts.write_buffer_size = 256;
  opts.wal_sync_every = 1;

  DB* db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  EXPECT_TRUE(db->Put("key", "v1").ok());
  for (int i = 0; i < 50; ++i) {
    EXPECT_TRUE(db->Put("other_" + std::to_string(i), "x").ok());
  }
  EXPECT_TRUE(db->Put("key", "v2").ok());
  delete db;

  db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  std::string val;
  EXPECT_TRUE(db->Get("key", &val).ok());
  EXPECT_EQ(val, std::string("v2"));
  delete db;
  CleanDir(path);
}

TEST(DBEdgeTest, DeletePersistedAfterSSTFlush) {
  std::string path = "/tmp/focuskv_edge_delete";
  CleanDir(path);

  Options opts;
  opts.create_if_missing = true;
  opts.write_buffer_size = 256;
  opts.wal_sync_every = 1;

  DB* db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  EXPECT_TRUE(db->Put("gone", "val").ok());
  for (int i = 0; i < 50; ++i) {
    EXPECT_TRUE(db->Put("pad_" + std::to_string(i), "y").ok());
  }
  EXPECT_TRUE(db->Delete("gone").ok());
  delete db;

  db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  std::string val;
  EXPECT_TRUE(db->Get("gone", &val).IsNotFound());
  delete db;
  CleanDir(path);
}

TEST(DBEdgeTest, GetMissReturnsNotFound) {
  std::string path = "/tmp/focuskv_edge_miss";
  CleanDir(path);

  Options opts;
  opts.create_if_missing = true;

  DB* db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  std::string val;
  EXPECT_TRUE(db->Get("never_existed", &val).IsNotFound());
  EXPECT_TRUE(db->Put("exists", "yes").ok());
  EXPECT_TRUE(db->Get("never_existed", &val).IsNotFound());
  delete db;
  CleanDir(path);
}

TEST(DBEdgeTest, GroupCommitSurvivesCleanClose) {
  std::string path = "/tmp/focuskv_edge_groupcommit";
  CleanDir(path);

  Options opts;
  opts.create_if_missing = true;
  opts.wal_sync_every = 32;

  DB* db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(db->Put("gc_" + std::to_string(i), "val").ok());
  }
  delete db;  // destructor should Sync()

  db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  std::string val;
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(db->Get("gc_" + std::to_string(i), &val).ok());
    EXPECT_EQ(val, std::string("val"));
  }
  delete db;
  CleanDir(path);
}

TEST(DBEdgeTest, ValueWithSpaces) {
  std::string path = "/tmp/focuskv_edge_spaces";
  CleanDir(path);

  Options opts;
  opts.create_if_missing = true;
  opts.wal_sync_every = 1;

  DB* db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  EXPECT_TRUE(db->Put("name", "Ayush Lohumi").ok());
  delete db;

  db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  std::string val;
  EXPECT_TRUE(db->Get("name", &val).ok());
  EXPECT_EQ(val, std::string("Ayush Lohumi"));
  delete db;
  CleanDir(path);
}

TEST(DBEdgeTest, ReopenEmptyDb) {
  std::string path = "/tmp/focuskv_edge_empty";
  CleanDir(path);

  Options opts;
  opts.create_if_missing = true;

  DB* db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  delete db;

  db = nullptr;
  EXPECT_TRUE(DB::Open(opts, path, &db).ok());
  std::string val;
  EXPECT_TRUE(db->Get("any", &val).IsNotFound());
  delete db;
  CleanDir(path);
}

}  // namespace focuskv
