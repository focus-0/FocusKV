#pragma once

#include <fstream>
#include <string>
#include "src/storage/memtable.h"
#include "src/utils/slice.h"
#include "src/utils/status.h"

namespace focuskv {

class WALWriter {
 public:
  explicit WALWriter(const std::string& filename);
  ~WALWriter();

  Status AddRecord(uint64_t seq, ValueType type, const Slice& key, const Slice& value);
  Status Sync();
  bool is_open() const { return file_.is_open(); }

 private:
  std::ofstream file_;
  std::string filename_;
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
