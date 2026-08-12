#include "src/network/server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <sstream>

namespace focuskv {

Server::Server(DB* db, int port) : db_(db), port_(port) {}

Server::~Server() {
  Stop();
}

Status Server::Start() {
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    return Status::IOError("Failed to create socket");
  }

  int opt = 1;
  setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(server_fd_);
    return Status::IOError("Failed to bind socket port " + std::to_string(port_));
  }

  if (listen(server_fd_, 128) < 0) {
    close(server_fd_);
    return Status::IOError("Failed to listen on socket");
  }

  running_ = true;
  listen_thread_ = std::thread(&Server::ListenLoop, this);
  return Status::OK();
}

void Server::Stop() {
  if (running_) {
    running_ = false;
    if (server_fd_ >= 0) {
      close(server_fd_);
      server_fd_ = -1;
    }
    if (listen_thread_.joinable()) {
      listen_thread_.join();
    }
  }
}

void Server::ListenLoop() {
  while (running_) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd >= 0) {
      std::thread(&Server::HandleClient, this, client_fd).detach();
    }
  }
}

void Server::HandleClient(int client_fd) {
  char buf[4096];
  while (running_) {
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) break;

    buf[n] = '\0';
    std::string request(buf, n);
    std::istringstream iss(request);

    std::string cmd, key, val;
    iss >> cmd;

    std::string response;
    if (cmd == "SET") {
      iss >> key;
      std::getline(iss, val);
      if (!val.empty() && val[0] == ' ') {
        val.erase(0, 1);
      }
      Status s = db_->Put(key, val);
      response = s.ok() ? "+OK\r\n" : "-ERR " + s.ToString() + "\r\n";
    } else if (cmd == "GET") {
      iss >> key;
      std::string out;
      Status s = db_->Get(key, &out);
      if (s.ok()) {
        response = "$" + std::to_string(out.size()) + "\r\n" + out + "\r\n";
      } else {
        response = "-ERR key not found\r\n";
      }
    } else if (cmd == "DEL") {
      iss >> key;
      Status s = db_->Delete(key);
      response = s.ok() ? ":1\r\n" : ":0\r\n";
    } else {
      response = "-ERR unknown command\r\n";
    }

    write(client_fd, response.data(), response.size());
  }

  close(client_fd);
}

}  // namespace focuskv
