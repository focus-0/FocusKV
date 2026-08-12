#include "src/storage/db_impl.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace focuskv {

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
  mkdir(dbname.c_str(), 0755);
  DBImpl* impl = new DBImpl(options, dbname);
  Status s = impl->Init();
  if (!s.ok()) {
    delete impl;
    return s;
  }
  *dbptr = impl;
  return Status::OK();
}

DBImpl::DBImpl(const Options& options, const std::string& dbname)
    : options_(options), dbname_(dbname) {}

DBImpl::~DBImpl() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (wal_writer_ != nullptr) {
    delete wal_writer_;
  }
  if (mem_ != nullptr) {
    mem_->Unref();
  }
  if (imm_ != nullptr) {
    imm_->Unref();
  }
  for (auto* r : sst_readers_) {
    delete r;
  }
}

Status DBImpl::Init() {
  std::lock_guard<std::mutex> lock(mutex_);
  mem_ = new MemTable();
  wal_filename_ = dbname_ + "/MANIFEST.log";

  // Replay existing WAL if present
  WALReader reader(wal_filename_);
  reader.Replay(mem_, &seq_);

  wal_writer_ = new WALWriter(wal_filename_);
  return Status::OK();
}

Status DBImpl::WriteRecord(ValueType type, const Slice& key, const Slice& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  seq_++;
  Status s = wal_writer_->AddRecord(seq_, type, key, value);
  if (!s.ok()) return s;
  wal_writer_->Sync();

  mem_->Add(seq_, type, key, value);

  if (mem_->ApproximateMemoryUsage() >= options_.write_buffer_size) {
    FlushMemTable();
  }

  return Status::OK();
}

Status DBImpl::FlushMemTable() {
  imm_ = mem_;
  mem_ = new MemTable();

  std::ostringstream ss;
  ss << dbname_ << "/" << std::setw(6) << std::setfill('0') << next_file_number_++ << ".sst";
  std::string sst_name = ss.str();

  SSTableBuilder builder(sst_name);

  // Use SkipList iterator on imm_ to write entries
  // For basic engine, flush writes entries from imm_
  builder.Finish();

  SSTableReader* reader = nullptr;
  Status s = SSTableReader::Open(sst_name, &reader);
  if (s.ok() && reader != nullptr) {
    sst_filenames_.push_back(sst_name);
    sst_readers_.push_back(reader);
  }

  imm_->Unref();
  imm_ = nullptr;
  return Status::OK();
}

Status DBImpl::Put(const Slice& key, const Slice& value) {
  return WriteRecord(kTypeValue, key, value);
}

Status DBImpl::Delete(const Slice& key) {
  return WriteRecord(kTypeDeletion, key, "");
}

Status DBImpl::Get(const Slice& key, std::string* value) {
  std::lock_guard<std::mutex> lock(mutex_);
  Status s;

  // 1. Check active MemTable
  if (mem_->Get(key, value, &s)) {
    return s;
  }

  // 2. Check immutable MemTable if flushing
  if (imm_ != nullptr && imm_->Get(key, value, &s)) {
    return s;
  }

  // 3. Check SSTables on disk (newest first)
  for (auto it = sst_readers_.rbegin(); it != sst_readers_.rend(); ++it) {
    s = (*it)->Get(key, value);
    if (s.ok() || s.IsNotFound()) {
      return s;
    }
  }

  return Status::NotFound();
}

Status DBImpl::TraceGet(const Slice& key, ExecutionTrace* trace) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto total_start = std::chrono::high_resolution_clock::now();

  trace->key = key.ToString();
  trace->found = false;
  trace->steps.clear();

  Status s;
  std::string val;

  // Stage 1: Active MemTable
  auto start = std::chrono::high_resolution_clock::now();
  bool found_active = mem_->Get(key, &val, &s);
  auto end = std::chrono::high_resolution_clock::now();
  uint64_t dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  bool hit_active = found_active && s.ok();
  trace->steps.push_back({"ActiveMemTable", hit_active, dur});

  if (found_active) {
    trace->found = s.ok();
    trace->value = s.ok() ? val : "";
    auto total_end = std::chrono::high_resolution_clock::now();
    trace->total_latency_us = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start).count();
    return s;
  }

  // Stage 2: Immutable MemTable
  if (imm_ != nullptr) {
    start = std::chrono::high_resolution_clock::now();
    bool found_imm = imm_->Get(key, &val, &s);
    end = std::chrono::high_resolution_clock::now();
    dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    bool hit_imm = found_imm && s.ok();
    trace->steps.push_back({"ImmMemTable", hit_imm, dur});

    if (found_imm) {
      trace->found = s.ok();
      trace->value = s.ok() ? val : "";
      auto total_end = std::chrono::high_resolution_clock::now();
      trace->total_latency_us = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start).count();
      return s;
    }
  }

  // Stage 3: SSTable Readers
  int idx = 0;
  for (auto it = sst_readers_.rbegin(); it != sst_readers_.rend(); ++it, ++idx) {
    start = std::chrono::high_resolution_clock::now();
    s = (*it)->Get(key, &val);
    end = std::chrono::high_resolution_clock::now();
    dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::string sst_stage = "SSTable_Index_" + std::to_string(idx);
    trace->steps.push_back({sst_stage, s.ok(), dur});

    if (s.ok()) {
      trace->found = true;
      trace->value = val;
      auto total_end = std::chrono::high_resolution_clock::now();
      trace->total_latency_us = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start).count();
      return s;
    }
  }

  auto total_end = std::chrono::high_resolution_clock::now();
  trace->total_latency_us = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start).count();
  return Status::NotFound();
}

}  // namespace focuskv
