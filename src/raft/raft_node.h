#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "src/storage/db.h"

namespace focuskv {

enum Role {
  kFollower = 0,
  kCandidate = 1,
  kLeader = 2
};

struct LogEntry {
  uint64_t term;
  std::string cmd;   // "SET", "DEL"
  std::string key;
  std::string value;
};

class RaftNode {
 public:
  RaftNode(int node_id, const std::vector<int>& peer_ports, int my_port, DB* db);
  ~RaftNode();

  void Start();
  void Stop();

  Role role() const { return role_.load(); }
  uint64_t current_term() const { return current_term_.load(); }
  int node_id() const { return node_id_; }
  bool is_leader() const { return role_.load() == kLeader; }

  // Execute write command across cluster majority
  Status Propose(const std::string& cmd, const std::string& key, const std::string& value);

 private:
  void RunLoop();
  void StartElection();
  void SendHeartbeats();

  int node_id_;
  std::vector<int> peer_ports_;
  int my_port_;
  DB* db_;

  std::atomic<Role> role_{kFollower};
  std::atomic<uint64_t> current_term_{0};
  int voted_for_{-1};

  std::vector<LogEntry> log_;
  uint64_t commit_index_{0};

  std::atomic<bool> running_{false};
  std::thread bg_thread_;
  std::mutex mutex_;
};

}  // namespace focuskv
