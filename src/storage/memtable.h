#pragma once

#include <atomic>
#include <functional>
#include <string>
#include "src/storage/arena.h"
#include "src/storage/skiplist.h"
#include "src/utils/coding.h"
#include "src/utils/slice.h"
#include "src/utils/status.h"

namespace focuskv {

enum ValueType {
  kTypeDeletion = 0x0,
  kTypeValue = 0x1
};

struct KeyComparator {
  int operator()(const char* a, const char* b) const;
};

class MemTable {
 public:
  MemTable();

  void Ref() { refs_.fetch_add(1, std::memory_order_relaxed); }
  void Unref() {
    if (refs_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete this;
    }
  }

  void Add(uint64_t seq, ValueType type, const Slice& key, const Slice& value);
  bool Get(const Slice& key, std::string* value, Status* status);
  void Iterate(const std::function<void(const Slice&, const Slice&, ValueType)>& fn) const;

  size_t ApproximateMemoryUsage() const { return arena_.MemoryUsage(); }

 private:
  ~MemTable() = default;

  KeyComparator comparator_;
  Arena arena_;
  SkipList<const char*, KeyComparator> table_;
  std::atomic<int> refs_{0};
};

}  // namespace focuskv
