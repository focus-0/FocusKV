#include "src/storage/db_impl.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "src/storage/manifest.h"

namespace focuskv {

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
  struct stat st;
  bool exists = stat(dbname.c_str(), &st) == 0;
  if (!exists && !options.create_if_missing) {
    return Status::IOError("Database directory does not exist: " + dbname);
  }
  if (!exists) {
    mkdir(dbname.c_str(), 0755);
  }

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
  std::unique_lock<std::shared_mutex> lock(mutex_);
  if (mem_ != nullptr && mem_->ApproximateMemoryUsage() > 0) {
    FlushMemTable();
  }
  if (wal_writer_ != nullptr) {
    wal_writer_->Sync();
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
  std::unique_lock<std::shared_mutex> lock(mutex_);
  mem_ = new MemTable();
  wal_filename_ = dbname_ + "/wal.log";

  ManifestData manifest;
  Status s = LoadManifest(dbname_, &manifest);
  if (!s.ok()) return s;

  next_file_number_ = manifest.next_file_number;

  WALReader reader(wal_filename_);
  s = reader.Replay(mem_, &seq_);
  if (!s.ok()) return s;
  if (seq_ < manifest.seq) {
    seq_ = manifest.seq;
  }

  wal_writer_ = new WALWriter(wal_filename_, options_.wal_sync_every);
  if (!wal_writer_->is_open()) {
    return Status::IOError("Failed to open WAL for writing");
  }

  if (!manifest.sst_files.empty()) {
    for (const auto& name : manifest.sst_files) {
      std::string path = dbname_ + "/" + name;
      SSTableReader* reader_ptr = nullptr;
      s = SSTableReader::Open(path, &reader_ptr);
      if (!s.ok()) return s;
      sst_filenames_.push_back(path);
      sst_readers_.push_back(reader_ptr);
    }
    return Status::OK();
  }

  return LoadSSTables();
}

Status DBImpl::LoadSSTables() {
  DIR* dir = opendir(dbname_.c_str());
  if (dir == nullptr) {
    return Status::IOError("Failed to open db directory");
  }

  std::vector<std::string> sst_files;
  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string name = entry->d_name;
    if (name.size() > 4 && name.substr(name.size() - 4) == ".sst") {
      sst_files.push_back(name);
    }
  }
  closedir(dir);

  std::sort(sst_files.begin(), sst_files.end());
  for (const auto& name : sst_files) {
    if (std::find(sst_filenames_.begin(), sst_filenames_.end(), dbname_ + "/" + name) !=
        sst_filenames_.end()) {
      continue;
    }
    std::string path = dbname_ + "/" + name;
    SSTableReader* reader = nullptr;
    Status s = SSTableReader::Open(path, &reader);
    if (!s.ok()) {
      return s;
    }
    sst_filenames_.push_back(path);
    sst_readers_.push_back(reader);

    uint64_t file_num = 0;
    if (name.size() >= 10) {
      file_num = static_cast<uint64_t>(std::stoul(name.substr(0, 6)));
    }
    if (file_num >= next_file_number_) {
      next_file_number_ = file_num + 1;
    }
  }
  return SaveCurrentManifest();
}

Status DBImpl::SaveCurrentManifest() {
  ManifestData manifest;
  manifest.seq = seq_;
  manifest.next_file_number = next_file_number_;
  for (const auto& path : sst_filenames_) {
    size_t pos = path.find_last_of('/');
    manifest.sst_files.push_back(pos == std::string::npos ? path : path.substr(pos + 1));
  }
  return SaveManifest(dbname_, manifest);
}

Status DBImpl::WriteRecord(ValueType type, const Slice& key, const Slice& value) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  seq_++;
  Status s = wal_writer_->AddRecord(seq_, type, key, value);
  if (!s.ok()) return s;

  mem_->Add(seq_, type, key, value);

  if (mem_->ApproximateMemoryUsage() >= options_.write_buffer_size) {
    s = FlushMemTable();
    if (!s.ok()) return s;
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
  imm_->Iterate([&](const Slice& key, const Slice& value, ValueType type) {
    if (type == kTypeValue) {
      builder.Add(key, value);
    } else {
      builder.Add(key, Slice());
    }
  });
  Status s = builder.Finish();
  if (!s.ok()) {
    imm_->Unref();
    imm_ = nullptr;
    return s;
  }

  SSTableReader* reader = nullptr;
  s = SSTableReader::Open(sst_name, &reader);
  if (!s.ok()) {
    imm_->Unref();
    imm_ = nullptr;
    return s;
  }

  sst_filenames_.push_back(sst_name);
  sst_readers_.push_back(reader);

  imm_->Unref();
  imm_ = nullptr;

  s = SaveCurrentManifest();
  if (!s.ok()) return s;

  return wal_writer_->Reset();
}

Status DBImpl::Put(const Slice& key, const Slice& value) {
  return WriteRecord(kTypeValue, key, value);
}

Status DBImpl::Delete(const Slice& key) {
  return WriteRecord(kTypeDeletion, key, "");
}

Status DBImpl::Get(const Slice& key, std::string* value) {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  Status s;

  if (mem_->Get(key, value, &s)) {
    return s;
  }

  if (imm_ != nullptr && imm_->Get(key, value, &s)) {
    return s;
  }

  for (auto it = sst_readers_.rbegin(); it != sst_readers_.rend(); ++it) {
    s = (*it)->Get(key, value);
    if (s.ok()) {
      if (value->empty()) {
        return Status::NotFound();
      }
      return s;
    }
  }

  return Status::NotFound();
}

}  // namespace focuskv
