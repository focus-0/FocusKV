#include "src/network/server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include "src/utils/test_harness.h"

namespace focuskv {

static std::string SendSocketCommand(int port, const std::string& cmd) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return "";

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    return "";
  }

  write(fd, cmd.data(), cmd.size());
  char buf[4096];
  ssize_t n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n <= 0) return "";
  buf[n] = '\0';
  return std::string(buf, n);
}

TEST(ServerTest, NetworkSocketCommands) {
  std::string db_path = "/tmp/focuskv_test_server_db";
  system(("rm -rf " + db_path).c_str());

  DB* db = nullptr;
  Options options;
  options.create_if_missing = true;
  EXPECT_TRUE(DB::Open(options, db_path, &db).ok());

  Server server(db, 9876);
  EXPECT_TRUE(server.Start().ok());

  // 1. SET command over TCP
  std::string resp = SendSocketCommand(9876, "SET user:10 Ayush\n");
  EXPECT_EQ(resp, std::string("+OK\r\n"));

  // 2. GET command over TCP
  resp = SendSocketCommand(9876, "GET user:10\n");
  EXPECT_EQ(resp, std::string("$5\r\nAyush\r\n"));

  // 3. TRACE command over TCP
  resp = SendSocketCommand(9876, "TRACE user:10\n");
  EXPECT_TRUE(resp.find("\"found\":true") != std::string::npos);

  // 4. DEL command over TCP
  resp = SendSocketCommand(9876, "DEL user:10\n");
  EXPECT_EQ(resp, std::string(":1\r\n"));

  // GET after DEL
  resp = SendSocketCommand(9876, "GET user:10\n");
  EXPECT_EQ(resp, std::string("-ERR key not found\r\n"));

  server.Stop();
  delete db;
  system(("rm -rf " + db_path).c_str());
}

}  // namespace focuskv
