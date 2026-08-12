#include "src/storage/sstable.h"
#include <cstdio>
#include <string>
#include "src/utils/test_harness.h"

namespace focuskv {

TEST(SSTableTest, BuildAndRead) {
  std::string sst_file = "/tmp/focuskv_test.sst";
  std::remove(sst_file.c_str());

  // 1. Build SSTable with sorted entries
  {
    SSTableBuilder builder(sst_file);
    builder.Add("key1", "value1");
    builder.Add("key2", "value2");
    builder.Add("key3", "value3");
    builder.Add("key4", "value4");
    EXPECT_TRUE(builder.Finish().ok());
  }

  // 2. Open SSTableReader and query keys
  SSTableReader* reader = nullptr;
  EXPECT_TRUE(SSTableReader::Open(sst_file, &reader).ok());
  EXPECT_TRUE(reader != nullptr);

  std::string val;
  EXPECT_TRUE(reader->Get("key1", &val).ok());
  EXPECT_EQ(val, std::string("value1"));

  EXPECT_TRUE(reader->Get("key3", &val).ok());
  EXPECT_EQ(val, std::string("value3"));

  // Non-existent key
  EXPECT_TRUE(reader->Get("key5", &val).IsNotFound());

  delete reader;
  std::remove(sst_file.c_str());
}

}  // namespace focuskv
