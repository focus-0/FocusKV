#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

#include "src/network/server.h"
#include "src/storage/db.h"

namespace {

volatile std::sig_atomic_t g_running = 1;

void HandleSignal(int) {
  g_running = 0;
}

int ParseIntArg(const char* arg, const char* key, int fallback) {
  std::string prefix = std::string("--") + key + "=";
  if (std::string(arg).find(prefix) == 0) {
    return std::stoi(std::string(arg).substr(prefix.size()));
  }
  return fallback;
}

std::string ParseStringArg(int argc, char** argv, const char* key, const std::string& fallback) {
  std::string prefix = std::string("--") + key + "=";
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]).find(prefix) == 0) {
      return std::string(argv[i]).substr(prefix.size());
    }
  }
  return fallback;
}

}  // namespace

int main(int argc, char** argv) {
  int node_id = 1;
  int client_port = 7001;
  std::string data_dir = "/tmp/focuskv_node";

  for (int i = 1; i < argc; ++i) {
    node_id = ParseIntArg(argv[i], "id", node_id);
    client_port = ParseIntArg(argv[i], "port", client_port);
  }
  data_dir = ParseStringArg(argc, argv, "data", data_dir);

  focuskv::DB* db = nullptr;
  focuskv::Options options;
  options.create_if_missing = true;
  focuskv::Status s = focuskv::DB::Open(options, data_dir, &db);
  if (!s.ok()) {
    std::cerr << "Failed to open DB: " << s.ToString() << std::endl;
    return 1;
  }

  focuskv::Server server(db, client_port);
  s = server.Start();
  if (!s.ok()) {
    std::cerr << "Failed to start server: " << s.ToString() << std::endl;
    delete db;
    return 1;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  std::cout << "FocusKV node " << node_id << " listening on " << client_port << std::endl;

  while (g_running) {
    sleep(1);
  }

  server.Stop();
  delete db;
  return 0;
}
