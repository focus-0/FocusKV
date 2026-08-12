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

TEST(ClusterTest, IndependentNodesDoNotShareData) {
  system("rm -rf /tmp/focuskv_cluster1 /tmp/focuskv_cluster2 /tmp/focuskv_cluster3");

  Options options;
  options.create_if_missing = true;

  DB *db1 = nullptr, *db2 = nullptr, *db3 = nullptr;
  EXPECT_TRUE(DB::Open(options, "/tmp/focuskv_cluster1", &db1).ok());
  EXPECT_TRUE(DB::Open(options, "/tmp/focuskv_cluster2", &db2).ok());
  EXPECT_TRUE(DB::Open(options, "/tmp/focuskv_cluster3", &db3).ok());

  Server s1(db1, 9201);
  Server s2(db2, 9202);
  Server s3(db3, 9203);
  EXPECT_TRUE(s1.Start().ok());
  EXPECT_TRUE(s2.Start().ok());
  EXPECT_TRUE(s3.Start().ok());

  std::string resp = SendSocketCommand(9201, "SET node1_key node1_value\n");
  EXPECT_EQ(resp, std::string("+OK\r\n"));
  resp = SendSocketCommand(9202, "SET node2_key node2_value\n");
  EXPECT_EQ(resp, std::string("+OK\r\n"));

  resp = SendSocketCommand(9201, "GET node1_key\n");
  EXPECT_EQ(resp, std::string("$11\r\nnode1_value\r\n"));
  resp = SendSocketCommand(9202, "GET node2_key\n");
  EXPECT_EQ(resp, std::string("$11\r\nnode2_value\r\n"));

  resp = SendSocketCommand(9202, "GET node1_key\n");
  EXPECT_EQ(resp, std::string("-ERR key not found\r\n"));
  resp = SendSocketCommand(9201, "GET node2_key\n");
  EXPECT_EQ(resp, std::string("-ERR key not found\r\n"));
  resp = SendSocketCommand(9203, "GET node1_key\n");
  EXPECT_EQ(resp, std::string("-ERR key not found\r\n"));

  s1.Stop();
  s2.Stop();
  s3.Stop();
  delete db1;
  delete db2;
  delete db3;
  system("rm -rf /tmp/focuskv_cluster1 /tmp/focuskv_cluster2 /tmp/focuskv_cluster3");
}

}  // namespace focuskv
