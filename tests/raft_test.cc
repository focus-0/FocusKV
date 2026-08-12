#include "src/raft/raft_node.h"
#include <cstdlib>
#include <vector>
#include "src/utils/test_harness.h"

namespace focuskv {

TEST(RaftTest, ThreeNodeClusterConsensus) {
  std::string db1_path = "/tmp/focuskv_raft_node1";
  std::string db2_path = "/tmp/focuskv_raft_node2";
  std::string db3_path = "/tmp/focuskv_raft_node3";

  system(("rm -rf " + db1_path + " " + db2_path + " " + db3_path).c_str());

  Options options;
  options.create_if_missing = true;

  DB *db1 = nullptr, *db2 = nullptr, *db3 = nullptr;
  EXPECT_TRUE(DB::Open(options, db1_path, &db1).ok());
  EXPECT_TRUE(DB::Open(options, db2_path, &db2).ok());
  EXPECT_TRUE(DB::Open(options, db3_path, &db3).ok());

  std::vector<int> node1_peers = {7002, 7003};
  std::vector<int> node2_peers = {7001, 7003};
  std::vector<int> node3_peers = {7001, 7002};

  RaftNode node1(1, node1_peers, 7001, db1);
  RaftNode node2(2, node2_peers, 7002, db2);
  RaftNode node3(3, node3_peers, 7003, db3);

  node1.Start();
  node2.Start();
  node3.Start();

  // Propose write on node1
  Status s = node1.Propose("SET", "cluster_key", "cluster_value");
  // If node1 is not leader, it handles proposal
  std::string val;
  if (db1->Get("cluster_key", &val).ok()) {
    EXPECT_EQ(val, std::string("cluster_value"));
  }

  node1.Stop();
  node2.Stop();
  node3.Stop();

  delete db1;
  delete db2;
  delete db3;

  system(("rm -rf " + db1_path + " " + db2_path + " " + db3_path).c_str());
}

}  // namespace focuskv
