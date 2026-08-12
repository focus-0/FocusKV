#include "src/storage/sstable.h"
#include <algorithm>
#include "src/utils/coding.h"

namespace focuskv {

SSTableBuilder::SSTableBuilder(const std::string& filename)
    : file_(filename, std::ios::binary | std::ios::out | std::ios::trunc),
      filename_(filename) {}

SSTableBuilder::~SSTableBuilder() {
  if (file_.is_open()) {
    file_.close();
  }
}

void SSTableBuilder::Add(const Slice& key, const Slice& value) {
  PutLengthPrefixedSlice(&current_block_, key);
  PutLengthPrefixedSlice(&current_block_, value);
  last_key_in_block_ = key.ToString();

  if (current_block_.size() >= kBlockSizeThreshold) {
    FlushBlock();
  }
}

void SSTableBuilder::FlushBlock() {
  if (current_block_.empty()) return;

  uint64_t offset = file_size_;
  uint64_t size = current_block_.size();

  file_.write(current_block_.data(), current_block_.size());
  file_size_ += size;

  index_entries_.push_back({last_key_in_block_, offset, size});
  current_block_.clear();
}

Status SSTableBuilder::Finish() {
  FlushBlock();

  uint64_t index_offset = file_size_;
  std::string index_buf;

  // Encode Index Entries
  for (const auto& entry : index_entries_) {
    PutLengthPrefixedSlice(&index_buf, entry.last_key);
    PutFixed64(&index_buf, entry.offset);
    PutFixed64(&index_buf, entry.size);
  }

  file_.write(index_buf.data(), index_buf.size());
  uint64_t index_size = index_buf.size();
  file_size_ += index_size;

  // Encode Footer: IndexOffset (8B) + IndexSize (8B) + MagicNumber (8B)
  std::string footer;
  PutFixed64(&footer, index_offset);
  PutFixed64(&footer, index_size);
  PutFixed64(&footer, kSSTableMagicNumber);

  file_.write(footer.data(), footer.size());
  file_size_ += footer.size();

  file_.flush();
  return Status::OK();
}

// SSTableReader
SSTableReader::SSTableReader(const std::string& filename, uint64_t index_offset, uint64_t index_size)
    : file_(filename, std::ios::binary | std::ios::in), filename_(filename) {}

SSTableReader::~SSTableReader() {
  if (file_.is_open()) {
    file_.close();
  }
}

Status SSTableReader::Open(const std::string& filename, SSTableReader** reader) {
  std::ifstream f(filename, std::ios::binary | std::ios::ate);
  if (!f.is_open()) {
    return Status::IOError("Failed to open SSTable file: " + filename);
  }

  uint64_t file_size = f.tellg();
  if (file_size < 24) {
    return Status::Corruption("SSTable file size too small");
  }

  // Read Footer at end of file
  f.seekg(file_size - 24);
  char footer_buf[24];
  f.read(footer_buf, 24);

  uint64_t index_offset = DecodeFixed64(footer_buf);
  uint64_t index_size = DecodeFixed64(footer_buf + 8);
  uint64_t magic = DecodeFixed64(footer_buf + 16);

  if (magic != kSSTableMagicNumber) {
    return Status::Corruption("Invalid SSTable magic number");
  }

  SSTableReader* r = new SSTableReader(filename, index_offset, index_size);
  Status s = r->ReadIndex(index_offset, index_size);
  if (!s.ok()) {
    delete r;
    return s;
  }

  *reader = r;
  return Status::OK();
}

Status SSTableReader::ReadIndex(uint64_t index_offset, uint64_t index_size) {
  if (!file_.is_open()) {
    return Status::IOError("SSTable file not open");
  }

  file_.seekg(index_offset);
  std::string index_buf;
  index_buf.resize(index_size);
  file_.read(&index_buf[0], index_size);

  Slice input(index_buf);
  while (!input.empty()) {
    Slice last_key;
    if (!GetLengthPrefixedSlice(&input, &last_key)) break;
    if (input.size() < 16) break;

    uint64_t offset = DecodeFixed64(input.data());
    input.remove_prefix(8);

    uint64_t size = DecodeFixed64(input.data());
    input.remove_prefix(8);

    index_entries_.push_back({last_key.ToString(), offset, size});
  }

  return Status::OK();
}

Status SSTableReader::Get(const Slice& key, std::string* value) {
  if (index_entries_.empty()) {
    return Status::NotFound();
  }

  // Binary search index entries for first block with last_key >= key
  int left = 0;
  int right = static_cast<int>(index_entries_.size()) - 1;
  int target_block = -1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    if (Slice(index_entries_[mid].last_key).compare(key) >= 0) {
      target_block = mid;
      right = mid - 1;
    } else {
      left = mid + 1;
    }
  }

  if (target_block == -1) {
    return Status::NotFound();
  }

  // Read target block
  const auto& entry = index_entries_[target_block];
  file_.seekg(entry.offset);
  std::string block_buf;
  block_buf.resize(entry.size);
  file_.read(&block_buf[0], entry.size);

  Slice block_input(block_buf);
  while (!block_input.empty()) {
    Slice k, v;
    if (!GetLengthPrefixedSlice(&block_input, &k)) break;
    if (!GetLengthPrefixedSlice(&block_input, &v)) break;

    if (k == key) {
      value->assign(v.data(), v.size());
      return Status::OK();
    }
  }

  return Status::NotFound();
}

}  // namespace focuskv
