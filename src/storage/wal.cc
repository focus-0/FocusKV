#include "src/storage/wal.h"

#include "src/utils/coding.h"

#include <fcntl.h>
#include <unistd.h>

namespace focuskv {

WALWriter::WALWriter(const std::string& filename, size_t sync_every)
    : filename_(filename), sync_every_(sync_every > 0 ? sync_every : 1) {
  fd_ = ::open(filename_.c_str(), O_CREAT | O_RDWR | O_APPEND, 0644);
}

WALWriter::~WALWriter() {
  if (fd_ >= 0) {
    Sync();
    ::close(fd_);
    fd_ = -1;
  }
}

Status WALWriter::WriteAll(const char* data, size_t len) {
  while (len > 0) {
    ssize_t n = ::write(fd_, data, len);
    if (n < 0) {
      return Status::IOError("WAL write failed");
    }
    data += n;
    len -= static_cast<size_t>(n);
  }
  return Status::OK();
}

Status WALWriter::AddRecord(uint64_t seq, ValueType type, const Slice& key, const Slice& value) {
  if (fd_ < 0) {
    return Status::IOError("WAL file is not open");
  }

  std::string record;
  PutFixed64(&record, seq);
  record.push_back(static_cast<char>(type));
  PutLengthPrefixedSlice(&record, key);
  PutLengthPrefixedSlice(&record, value);

  std::string header;
  PutFixed32(&header, static_cast<uint32_t>(record.size()));

  Status s = WriteAll(header.data(), header.size());
  if (!s.ok()) return s;
  s = WriteAll(record.data(), record.size());
  if (!s.ok()) return s;

  writes_since_sync_++;
  if (writes_since_sync_ >= sync_every_) {
    return Sync();
  }
  return Status::OK();
}

Status WALWriter::Sync() {
  if (fd_ < 0) {
    return Status::IOError("WAL file is not open");
  }
  writes_since_sync_ = 0;
  if (::fsync(fd_) != 0) {
    return Status::IOError("WAL fsync failed");
  }
  return Status::OK();
}

Status WALWriter::Reset() {
  if (fd_ >= 0) {
    Sync();
    ::close(fd_);
    fd_ = -1;
  }
  std::remove(filename_.c_str());
  fd_ = ::open(filename_.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd_ < 0) {
    return Status::IOError("Failed to reset WAL");
  }
  writes_since_sync_ = 0;
  return Status::OK();
}

WALReader::WALReader(const std::string& filename)
    : file_(filename, std::ios::binary | std::ios::in), filename_(filename) {}

WALReader::~WALReader() {
  if (file_.is_open()) {
    file_.close();
  }
}

Status WALReader::Replay(MemTable* memtable, uint64_t* max_seq) {
  if (!file_.is_open()) {
    if (max_seq != nullptr) {
      *max_seq = 0;
    }
    return Status::OK();
  }

  uint64_t highest_seq = 0;
  while (file_.good() && !file_.eof()) {
    char header_buf[4];
    file_.read(header_buf, sizeof(header_buf));
    if (file_.gcount() < static_cast<std::streamsize>(sizeof(header_buf))) {
      break;
    }

    uint32_t record_len = DecodeFixed32(header_buf);
    std::string record_buf;
    record_buf.resize(record_len);
    file_.read(&record_buf[0], record_len);

    if (file_.gcount() < static_cast<std::streamsize>(record_len)) {
      break;
    }

    Slice input(record_buf);
    if (input.size() < 9) break;

    uint64_t seq = DecodeFixed64(input.data());
    input.remove_prefix(8);

    ValueType type = static_cast<ValueType>(input[0]);
    input.remove_prefix(1);

    Slice key, value;
    if (!GetLengthPrefixedSlice(&input, &key)) break;
    if (!GetLengthPrefixedSlice(&input, &value)) break;

    memtable->Add(seq, type, key, value);
    if (seq > highest_seq) {
      highest_seq = seq;
    }
  }

  if (max_seq != nullptr) {
    *max_seq = highest_seq;
  }

  return Status::OK();
}

}  // namespace focuskv
