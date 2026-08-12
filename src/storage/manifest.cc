#include "src/storage/manifest.h"

#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

namespace focuskv {

static std::string ManifestPath(const std::string& dbname) {
  return dbname + "/MANIFEST";
}

static Status SyncPath(const std::string& path) {
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    return Status::IOError("Failed to open for fsync: " + path);
  }
  if (::fsync(fd) != 0) {
    ::close(fd);
    return Status::IOError("Failed to fsync: " + path);
  }
  ::close(fd);
  return Status::OK();
}

Status LoadManifest(const std::string& dbname, ManifestData* out) {
  std::ifstream in(ManifestPath(dbname));
  if (!in.is_open()) {
    out->seq = 0;
    out->next_file_number = 1;
    out->sst_files.clear();
    return Status::OK();
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    if (line.rfind("seq=", 0) == 0) {
      out->seq = static_cast<uint64_t>(std::stoull(line.substr(4)));
    } else if (line.rfind("next_file=", 0) == 0) {
      out->next_file_number = static_cast<uint64_t>(std::stoull(line.substr(10)));
    } else if (line.rfind("sst=", 0) == 0) {
      out->sst_files.push_back(line.substr(4));
    }
  }
  return Status::OK();
}

Status SaveManifest(const std::string& dbname, const ManifestData& data) {
  std::string tmp = ManifestPath(dbname) + ".tmp";
  std::ofstream out(tmp, std::ios::trunc);
  if (!out.is_open()) {
    return Status::IOError("Failed to write manifest");
  }

  out << "seq=" << data.seq << "\n";
  out << "next_file=" << data.next_file_number << "\n";
  for (const auto& sst : data.sst_files) {
    out << "sst=" << sst << "\n";
  }
  out.flush();
  out.close();

  Status s = SyncPath(tmp);
  if (!s.ok()) return s;

  if (std::rename(tmp.c_str(), ManifestPath(dbname).c_str()) != 0) {
    return Status::IOError("Failed to rename manifest");
  }

  s = SyncPath(ManifestPath(dbname));
  if (!s.ok()) return s;

  return SyncPath(dbname);
}

}  // namespace focuskv
