#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "src/storage/bloom_filter.h"
#include "src/storage/memtable.h"
#include "src/utils/slice.h"
#include "src/utils/status.h"

namespace focuskv {

static const uint64_t kSSTableMagicNumber = 0x666f6375736b7631ULL;  // "focuskv1"
static const size_t kSSTableFooterSize = 40;

struct BlockIndexEntry {
  std::string last_key;
  uint64_t offset;
  uint64_t size;
};

class SSTableBuilder {
 public:
  explicit SSTableBuilder(const std::string& filename);
  ~SSTableBuilder();

  void Add(const Slice& key, const Slice& value);
  Status Finish();
  uint64_t FileSize() const { return file_size_; }

 private:
  void FlushBlock();

  std::ofstream file_;
  std::string filename_;
  std::string current_block_;
  std::string last_key_in_block_;
  std::vector<BlockIndexEntry> index_entries_;
  BloomFilter bloom_;
  size_t key_count_{0};
  uint64_t file_size_{0};
  static const size_t kBlockSizeThreshold = 4096;
};

class SSTableReader {
 public:
  static Status Open(const std::string& filename, SSTableReader** reader);
  ~SSTableReader();

  Status Get(const Slice& key, std::string* value);

 private:
  SSTableReader(const std::string& filename, uint64_t index_offset, uint64_t index_size);
  Status ReadIndex(uint64_t index_offset, uint64_t index_size);
  Status ReadBloom(uint64_t bloom_offset, uint64_t bloom_size);

  std::ifstream file_;
  std::string filename_;
  std::vector<BlockIndexEntry> index_entries_;
  BloomFilter bloom_;
};

}  // namespace focuskv
