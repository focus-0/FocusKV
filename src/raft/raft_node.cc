#include "src/raft/raft_node.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace focuskv {

RaftNode::RaftNode(int node_id, const std::vector<int>& peer_ports, int my_port, DB* db)
    : node_id_(node_id), peer_ports_(peer_ports), my_port_(my_port), db_(db) {}

RaftNode::~RaftNode() {
  Stop();
}

void RaftNode::Start() {
  running_ = true;
  bg_thread_ = std::thread(&RaftNode::RunLoop, this);
}

void RaftNode::Stop() {
  if (running_) {
    running_ = false;
    if (bg_thread_.joinable()) {
      bg_thread_.join();
    }
  }
}

void RaftNode::RunLoop() {
  while (running_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::lock_guard<std::mutex> lock(mutex_);

    if (role_ == kLeader) {
      SendHeartbeats();
    }
  }
}

void RaftNode::StartElection() {
  role_ = kCandidate;
  current_term_++;
  voted_for_ = node_id_;

  int votes = 1; // Vote for self
  int total_nodes = static_cast<int>(peer_ports_.size()) + 1;

  if (votes > total_nodes / 2) {
    role_ = kLeader;
  }
}

void RaftNode::SendHeartbeats() {
  // Leader sends heartbeats to keep followers alive
}

Status RaftNode::Propose(const std::string& cmd, const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (role_ != kLeader) {
    return Status::InvalidArgument("Not the leader");
  }

  LogEntry entry{current_term_, cmd, key, value};
  log_.push_back(entry);
  commit_index_ = log_.size();

  // Apply to local storage engine
  if (cmd == "SET") {
    db_->Put(key, value);
  } else if (cmd == "DEL") {
    db_->Delete(key);
  }

  return Status::OK();
}

}  // namespace focuskv
