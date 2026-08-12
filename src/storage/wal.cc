#include "src/storage/wal.h"
#include "src/utils/coding.h"

namespace focuskv {

WALWriter::WALWriter(const std::string& filename)
    : file_(filename, std::ios::binary | std::ios::app | std::ios::out),
      filename_(filename) {}

WALWriter::~WALWriter() {
  if (file_.is_open()) {
    file_.close();
  }
}

Status WALWriter::AddRecord(uint64_t seq, ValueType type, const Slice& key, const Slice& value) {
  if (!file_.is_open()) {
    return Status::IOError("WAL file is not open");
  }

  std::string record;
  PutFixed64(&record, seq);
  record.push_back(static_cast<char>(type));
  PutLengthPrefixedSlice(&record, key);
  PutLengthPrefixedSlice(&record, value);

  uint32_t record_len = static_cast<uint32_t>(record.size());
  std::string header;
  PutFixed32(&header, record_len);

  file_.write(header.data(), header.size());
  file_.write(record.data(), record.size());

  return Status::OK();
}

Status WALWriter::Sync() {
  if (!file_.is_open()) {
    return Status::IOError("WAL file is not open");
  }
  file_.flush();
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
    return Status::IOError("WAL file cannot be opened for reading");
  }

  uint64_t highest_seq = 0;
  while (file_.good() && !file_.eof()) {
    char header_buf[4];
    file_.read(header_buf, sizeof(header_buf));
    if (file_.gcount() < static_cast<std::streamsize>(sizeof(header_buf))) {
      break; // Clean EOF
    }

    uint32_t record_len = DecodeFixed32(header_buf);
    std::string record_buf;
    record_buf.resize(record_len);
    file_.read(&record_buf[0], record_len);

    if (file_.gcount() < static_cast<std::streamsize>(record_len)) {
      break; // Incomplete tail record (crash mid-write), stop replay gracefully
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
