#include "src/storage/memtable.h"
#include "src/utils/test_harness.h"

namespace focuskv {

TEST(MemTableTest, SimpleAddAndGet) {
  MemTable* memtable = new MemTable();

  memtable->Add(1, kTypeValue, "key1", "val1");
  memtable->Add(2, kTypeValue, "key2", "val2");

  std::string val;
  Status s;

  EXPECT_TRUE(memtable->Get("key1", &val, &s));
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(val, std::string("val1"));

  EXPECT_TRUE(memtable->Get("key2", &val, &s));
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(val, std::string("val2"));

  EXPECT_FALSE(memtable->Get("key3", &val, &s));

  memtable->Unref();
}

TEST(MemTableTest, OverwriteAndDeletion) {
  MemTable* memtable = new MemTable();

  memtable->Add(1, kTypeValue, "user", "Ayush");

  std::string val;
  Status s;
  EXPECT_TRUE(memtable->Get("user", &val, &s));
  EXPECT_EQ(val, std::string("Ayush"));

  memtable->Add(2, kTypeValue, "user", "Lohumi");
  EXPECT_TRUE(memtable->Get("user", &val, &s));
  EXPECT_EQ(val, std::string("Lohumi"));

  memtable->Add(3, kTypeDeletion, "user", "");
  EXPECT_TRUE(memtable->Get("user", &val, &s));
  EXPECT_TRUE(s.IsNotFound());

  memtable->Unref();
}

}  // namespace focuskv
