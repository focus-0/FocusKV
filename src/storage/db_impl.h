#pragma once

#include <shared_mutex>
#include <string>
#include <vector>

#include "src/storage/db.h"
#include "src/storage/memtable.h"
#include "src/storage/sstable.h"
#include "src/storage/wal.h"

namespace focuskv {

class DBImpl : public DB {
 public:
  DBImpl(const Options& options, const std::string& dbname);
  ~DBImpl() override;

  Status Init();

  Status Put(const Slice& key, const Slice& value) override;
  Status Get(const Slice& key, std::string* value) override;
  Status Delete(const Slice& key) override;

 private:
  Status WriteRecord(ValueType type, const Slice& key, const Slice& value);
  Status FlushMemTable();
  Status LoadSSTables();
  Status SaveCurrentManifest();

  Options options_;
  std::string dbname_;
  std::shared_mutex mutex_;

  uint64_t seq_{0};
  MemTable* mem_{nullptr};
  MemTable* imm_{nullptr};

  WALWriter* wal_writer_{nullptr};
  std::string wal_filename_;

  std::vector<std::string> sst_filenames_;
  std::vector<SSTableReader*> sst_readers_;
  uint64_t next_file_number_{1};
};

}  // namespace focuskv
