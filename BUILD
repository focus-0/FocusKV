load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "utils",
    srcs = ["src/utils/coding.cc"],
    hdrs = [
        "src/utils/coding.h",
        "src/utils/slice.h",
        "src/utils/status.h",
        "src/utils/test_harness.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "arena",
    srcs = ["src/storage/arena.cc"],
    hdrs = ["src/storage/arena.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "skiplist",
    hdrs = ["src/storage/skiplist.h"],
    deps = [":arena"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "memtable",
    srcs = ["src/storage/memtable.cc"],
    hdrs = ["src/storage/memtable.h"],
    deps = [
        ":arena",
        ":skiplist",
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "wal",
    srcs = ["src/storage/wal.cc"],
    hdrs = ["src/storage/wal.h"],
    deps = [
        ":memtable",
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sstable",
    srcs = ["src/storage/sstable.cc"],
    hdrs = ["src/storage/sstable.h"],
    deps = [
        ":memtable",
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "db",
    srcs = ["src/storage/db_impl.cc"],
    hdrs = [
        "src/storage/db.h",
        "src/storage/db_impl.h",
        "src/storage/query_tracer.h",
    ],
    deps = [
        ":memtable",
        ":sstable",
        ":utils",
        ":wal",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "server",
    srcs = ["src/network/server.cc"],
    hdrs = ["src/network/server.h"],
    deps = [
        ":db",
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "raft",
    srcs = ["src/raft/raft_node.cc"],
    hdrs = ["src/raft/raft_node.h"],
    deps = [
        ":db",
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "memtable_test",
    srcs = ["tests/memtable_test.cc"],
    deps = [
        ":memtable",
        ":utils",
    ],
)

cc_test(
    name = "wal_test",
    srcs = ["tests/wal_test.cc"],
    deps = [
        ":utils",
        ":wal",
    ],
)

cc_test(
    name = "sstable_test",
    srcs = ["tests/sstable_test.cc"],
    deps = [
        ":sstable",
        ":utils",
    ],
)

cc_test(
    name = "db_test",
    srcs = ["tests/db_test.cc"],
    deps = [
        ":db",
        ":utils",
    ],
)

cc_test(
    name = "server_test",
    srcs = ["tests/server_test.cc"],
    deps = [
        ":server",
        ":utils",
    ],
)

cc_test(
    name = "raft_test",
    srcs = ["tests/raft_test.cc"],
    deps = [
        ":raft",
        ":utils",
    ],
)

cc_binary(
    name = "db_bench",
    srcs = ["benchmarks/db_bench.cc"],
    deps = [
        ":db",
        ":utils",
    ],
)
