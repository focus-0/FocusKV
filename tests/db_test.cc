#include "src/storage/db.h"
#include <cstdlib>
#include <string>
#include "src/utils/test_harness.h"

namespace focuskv {

TEST(DBTest, BasicPutGetDelete) {
  std::string db_path = "/tmp/focuskv_test_db";
  system(("rm -rf " + db_path).c_str());

  DB* db = nullptr;
  Options options;
  options.create_if_missing = true;

  EXPECT_TRUE(DB::Open(options, db_path, &db).ok());
  EXPECT_TRUE(db != nullptr);

  EXPECT_TRUE(db->Put("user:1", "Ayush").ok());
  EXPECT_TRUE(db->Put("user:2", "Lohumi").ok());

  std::string val;
  EXPECT_TRUE(db->Get("user:1", &val).ok());
  EXPECT_EQ(val, std::string("Ayush"));

  EXPECT_TRUE(db->Get("user:2", &val).ok());
  EXPECT_EQ(val, std::string("Lohumi"));

  EXPECT_TRUE(db->Delete("user:1").ok());
  EXPECT_TRUE(db->Get("user:1", &val).IsNotFound());

  delete db;

  DB* reopened_db = nullptr;
  EXPECT_TRUE(DB::Open(options, db_path, &reopened_db).ok());

  EXPECT_TRUE(reopened_db->Get("user:1", &val).IsNotFound());
  EXPECT_TRUE(reopened_db->Get("user:2", &val).ok());
  EXPECT_EQ(val, std::string("Lohumi"));

  delete reopened_db;
  system(("rm -rf " + db_path).c_str());
}

TEST(DBTest, MemTableFlushToSSTable) {
  std::string db_path = "/tmp/focuskv_flush_db";
  system(("rm -rf " + db_path).c_str());

  DB* db = nullptr;
  Options options;
  options.create_if_missing = true;
  options.write_buffer_size = 256;

  EXPECT_TRUE(DB::Open(options, db_path, &db).ok());
  for (int i = 0; i < 100; ++i) {
    std::string key = "flush_key_" + std::to_string(i);
    std::string val = "flush_val_" + std::to_string(i);
    EXPECT_TRUE(db->Put(key, val).ok());
  }

  delete db;

  DB* reopened = nullptr;
  EXPECT_TRUE(DB::Open(options, db_path, &reopened).ok());
  std::string val;
  EXPECT_TRUE(reopened->Get("flush_key_50", &val).ok());
  EXPECT_EQ(val, std::string("flush_val_50"));

  delete reopened;
  system(("rm -rf " + db_path).c_str());
}

}  // namespace focuskv
