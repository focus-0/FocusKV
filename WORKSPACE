workspace(name = "focus_kv")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_cc",
    urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.17/rules_cc-0.0.17.tar.gz"],
    sha256 = "07119ff39eef7f8cc3d7e7d6ed1f496350325d7b51b72eebc0b05bdf9ff14ef0",
)

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")
rules_cc_dependencies()

http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip"],
    strip_prefix = "googletest-1.14.0",
    sha256 = "1f357c27ca4f8885563e755a3a9774e1e01650143f7280e39fae56805ee59b98",
)