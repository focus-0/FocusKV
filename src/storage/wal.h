#pragma once

#include <fstream>
#include <string>

#include "src/storage/memtable.h"
#include "src/utils/slice.h"
#include "src/utils/status.h"

namespace focuskv {

class WALWriter {
 public:
  WALWriter(const std::string& filename, size_t sync_every = 32);
  ~WALWriter();

  Status AddRecord(uint64_t seq, ValueType type, const Slice& key, const Slice& value);
  Status Sync();
  Status Reset();
  bool is_open() const { return fd_ >= 0; }

 private:
  Status WriteAll(const char* data, size_t len);

  int fd_{-1};
  std::string filename_;
  size_t sync_every_;
  size_t writes_since_sync_{0};
};

class WALReader {
 public:
  explicit WALReader(const std::string& filename);
  ~WALReader();

  Status Replay(MemTable* memtable, uint64_t* max_seq);

 private:
  std::ifstream file_;
  std::string filename_;
};

}  // namespace focuskv
