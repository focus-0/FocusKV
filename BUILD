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
    hdrs = [
        "src/storage/bloom_filter.h",
        "src/storage/sstable.h",
    ],
    deps = [
        ":memtable",
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "manifest",
    srcs = ["src/storage/manifest.cc"],
    hdrs = ["src/storage/manifest.h"],
    deps = [":utils"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "db",
    srcs = ["src/storage/db_impl.cc"],
    hdrs = [
        "src/storage/db.h",
        "src/storage/db_impl.h",
    ],
    deps = [
        ":manifest",
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
    name = "db_edge_test",
    srcs = ["tests/db_edge_test.cc"],
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
    name = "cluster_test",
    srcs = ["tests/cluster_test.cc"],
    deps = [
        ":server",
        ":utils",
    ],
)

cc_binary(
    name = "focuskv_node",
    srcs = ["cmd/focuskv_node.cc"],
    deps = [
        ":db",
        ":server",
        ":utils",
    ],
)
