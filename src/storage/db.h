#pragma once

#include <string>

#include "src/utils/slice.h"
#include "src/utils/status.h"

namespace focuskv {

struct Options {
  bool create_if_missing = true;
  size_t write_buffer_size = 4 * 1024 * 1024;
  size_t wal_sync_every = 32;  // group commit: fsync every N writes (Sync on close always)
};

class DB {
 public:
  static Status Open(const Options& options, const std::string& dbname, DB** dbptr);

  virtual Status Put(const Slice& key, const Slice& value) = 0;
  virtual Status Get(const Slice& key, std::string* value) = 0;
  virtual Status Delete(const Slice& key) = 0;

  virtual ~DB() = default;
};

}  // namespace focuskv
