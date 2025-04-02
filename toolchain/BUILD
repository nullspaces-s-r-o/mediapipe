load(":cc_toolchain_config.bzl", "cc_toolchain_config")
load(":rock_toolchain_config.bzl", "rock_toolchain_config")

package(default_visibility = ["//visibility:public"])

cc_toolchain_suite(
    name = "coral_suite",
    toolchains = {
        "aarch64": ":aarch64_toolchain",
    },
)

filegroup(name = "empty")

cc_toolchain(
    name = "aarch64_toolchain",
    toolchain_identifier = "aarch64-toolchain",
    toolchain_config = ":aarch64_toolchain_config",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
)

cc_toolchain_config(name = "aarch64_toolchain_config")

# Rock
cc_toolchain_suite(
    name = "rock_suite",
    toolchains = {
        "aarch64": ":rock_toolchain",
    },
)

cc_toolchain(
    name = "rock_toolchain",
    toolchain_identifier = "rock-toolchain",
    toolchain_config = ":rock_toolchain_config",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
)

rock_toolchain_config(name = "rock_toolchain_config")