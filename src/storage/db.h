#pragma once

#include <string>

#include "src/storage/query_tracer.h"
#include "src/utils/slice.h"
#include "src/utils/status.h"

namespace focuskv {

struct Options {
  bool create_if_missing = true;
  size_t write_buffer_size = 4 * 1024 * 1024; // 4MB default MemTable limit
};

class DB {
 public:
  static Status Open(const Options& options, const std::string& dbname, DB** dbptr);

  virtual Status Put(const Slice& key, const Slice& value) = 0;
  virtual Status Get(const Slice& key, std::string* value) = 0;
  virtual Status Delete(const Slice& key) = 0;

  // Novelty Feature: LSM Query Inspector
  virtual Status TraceGet(const Slice& key, ExecutionTrace* trace) = 0;

  virtual ~DB() = default;
};

}  // namespace focuskv
