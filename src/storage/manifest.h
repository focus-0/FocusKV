#pragma once

#include <string>
#include <vector>

#include "src/utils/status.h"

namespace focuskv {

struct ManifestData {
  uint64_t seq{0};
  uint64_t next_file_number{1};
  std::vector<std::string> sst_files;
};

Status LoadManifest(const std::string& dbname, ManifestData* out);
Status SaveManifest(const std::string& dbname, const ManifestData& data);

}  // namespace focuskv
