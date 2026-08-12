#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace focuskv {

struct TraceStep {
  std::string stage;     // e.g., "ActiveMemTable", "ImmMemTable", "SSTable_000001.sst"
  bool hit;              // True if key was found in this stage
  uint64_t latency_us;   // Microseconds spent in this stage
};

struct ExecutionTrace {
  std::string key;
  bool found;
  std::string value;
  uint64_t total_latency_us;
  std::vector<TraceStep> steps;
};

}  // namespace focuskv
