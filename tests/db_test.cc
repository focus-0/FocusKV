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

  // 1. Open DB
  EXPECT_TRUE(DB::Open(options, db_path, &db).ok());
  EXPECT_TRUE(db != nullptr);

  // 2. Put & Get
  EXPECT_TRUE(db->Put("user:1", "Ayush").ok());
  EXPECT_TRUE(db->Put("user:2", "Lohumi").ok());

  std::string val;
  EXPECT_TRUE(db->Get("user:1", &val).ok());
  EXPECT_EQ(val, std::string("Ayush"));

  EXPECT_TRUE(db->Get("user:2", &val).ok());
  EXPECT_EQ(val, std::string("Lohumi"));

  // 3. TraceGet (Query Inspector Novelty Feature)
  ExecutionTrace trace;
  EXPECT_TRUE(db->TraceGet("user:1", &trace).ok());
  EXPECT_TRUE(trace.found);
  EXPECT_EQ(trace.value, std::string("Ayush"));
  EXPECT_TRUE(!trace.steps.empty());
  EXPECT_EQ(trace.steps[0].stage, std::string("ActiveMemTable"));
  EXPECT_TRUE(trace.steps[0].hit);

  // 4. Delete
  EXPECT_TRUE(db->Delete("user:1").ok());
  EXPECT_TRUE(db->Get("user:1", &val).IsNotFound());

  delete db;

  // 5. Reopen DB and verify WAL crash recovery
  DB* reopened_db = nullptr;
  EXPECT_TRUE(DB::Open(options, db_path, &reopened_db).ok());

  EXPECT_TRUE(reopened_db->Get("user:1", &val).IsNotFound());
  EXPECT_TRUE(reopened_db->Get("user:2", &val).ok());
  EXPECT_EQ(val, std::string("Lohumi"));

  delete reopened_db;
  system(("rm -rf " + db_path).c_str());
}

}  // namespace focuskv
