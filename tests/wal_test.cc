#include "src/storage/wal.h"
#include <cstdio>
#include "src/utils/test_harness.h"

namespace focuskv {

TEST(WALTest, AppendAndReplay) {
  std::string test_log = "/tmp/focuskv_test_wal.log";
  std::remove(test_log.c_str());

  // 1. Write records to WAL
  {
    WALWriter writer(test_log);
    EXPECT_TRUE(writer.is_open());
    EXPECT_TRUE(writer.AddRecord(1, kTypeValue, "k1", "v1").ok());
    EXPECT_TRUE(writer.AddRecord(2, kTypeValue, "k2", "v2").ok());
    EXPECT_TRUE(writer.AddRecord(3, kTypeDeletion, "k1", "").ok());
    EXPECT_TRUE(writer.Sync().ok());
  }

  // 2. Replay log into a fresh MemTable
  MemTable* memtable = new MemTable();
  uint64_t max_seq = 0;
  {
    WALReader reader(test_log);
    EXPECT_TRUE(reader.Replay(memtable, &max_seq).ok());
  }

  // 3. Verify replayed records
  EXPECT_EQ(max_seq, 3ULL);

  std::string val;
  Status s;

  // k1 was deleted by tombstone (seq 3)
  EXPECT_TRUE(memtable->Get("k1", &val, &s));
  EXPECT_TRUE(s.IsNotFound());

  // k2 is present
  EXPECT_TRUE(memtable->Get("k2", &val, &s));
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(val, std::string("v2"));

  memtable->Unref();
  std::remove(test_log.c_str());
}

}  // namespace focuskv
