# Description:
#   Bazel workspace file for twim.

workspace(name = "ru_eustas_twim")

maven_jar(
    name = "junit_junit",
    artifact = "junit:junit:4.12",
)

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "io_bazel_rules_closure",
    commit = "6794c2755113e967394eec53ed4ceb6c7eb4d065",
    shallow_since = "1559345009 -0700",
    remote = "https://github.com/bazelbuild/rules_closure.git",
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_repositories")
closure_repositories()
