#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "src/storage/db.h"

int main(int argc, char** argv) {
  std::string db_path = "/tmp/focuskv_bench_db";
  system(("rm -rf " + db_path).c_str());

  focuskv::DB* db = nullptr;
  focuskv::Options options;
  options.create_if_missing = true;

  focuskv::Status s = focuskv::DB::Open(options, db_path, &db);
  if (!s.ok()) {
    std::cerr << "Failed to open benchmark DB: " << s.ToString() << std::endl;
    return 1;
  }

  const int kNumOps = 50000;
  std::cout << "==========================================================" << std::endl;
  std::cout << " FocusKV Benchmark Suite (db_bench)" << std::endl;
  std::cout << " Operations: " << kNumOps << " keys" << std::endl;
  std::cout << "==========================================================" << std::endl;

  // 1. Sequential Write Benchmark
  auto start_write = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kNumOps; ++i) {
    std::string key = "key_" + std::to_string(i);
    std::string val = "value_" + std::to_string(i);
    db->Put(key, val);
  }
  auto end_write = std::chrono::high_resolution_clock::now();
  double write_ms = std::chrono::duration<double, std::milli>(end_write - start_write).count();
  double write_ops_sec = (kNumOps / write_ms) * 1000.0;

  std::cout << "[WRITE] Sequential Put: " << kNumOps << " ops in "
            << write_ms << " ms (" << static_cast<uint64_t>(write_ops_sec)
            << " ops/sec)" << std::endl;

  // 2. Random Read Benchmark
  auto start_read = std::chrono::high_resolution_clock::now();
  std::string read_val;
  int hits = 0;
  for (int i = 0; i < kNumOps; ++i) {
    int target_id = rand() % kNumOps;
    std::string key = "key_" + std::to_string(target_id);
    if (db->Get(key, &read_val).ok()) {
      hits++;
    }
  }
  auto end_read = std::chrono::high_resolution_clock::now();
  double read_ms = std::chrono::duration<double, std::milli>(end_read - start_read).count();
  double read_ops_sec = (kNumOps / read_ms) * 1000.0;
  double avg_read_latency_us = (read_ms * 1000.0) / kNumOps;

  std::cout << "[READ ] Random Get    : " << kNumOps << " ops in "
            << read_ms << " ms (" << static_cast<uint64_t>(read_ops_sec)
            << " ops/sec, avg " << avg_read_latency_us << " us/op)" << std::endl;

  std::cout << "==========================================================" << std::endl;

  delete db;
  system(("rm -rf " + db_path).c_str());
  return 0;
}
