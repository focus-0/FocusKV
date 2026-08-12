#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "src/storage/db.h"

namespace focuskv {

class Server {
 public:
  Server(DB* db, int port);
  ~Server();

  Status Start();
  void Stop();
  int port() const { return port_; }

 private:
  void ListenLoop();
  void HandleClient(int client_fd);

  DB* db_;
  int port_;
  int server_fd_{-1};
  std::atomic<bool> running_{false};
  std::thread listen_thread_;
};

}  // namespace focuskv
