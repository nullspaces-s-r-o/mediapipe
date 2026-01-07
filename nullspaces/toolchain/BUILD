package(default_visibility = ["//visibility:public"])

cc_toolchain_suite(
    name = "dad_suite",
    toolchains = {
        "aarch64|rock-gcc": ":rock_toolchain",
        "aarch64|coral-gcc": ":coral_toolchain",
    },
)

# Rock
load(":rock_toolchain_config.bzl", "cc_toolchain_config")
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

cc_toolchain_config(name = "rock_toolchain_config")

# Coral
load(":coral_toolchain_config.bzl", "cc_toolchain_config")
cc_toolchain(
    name = "coral_toolchain",
    toolchain_identifier = "coral-toolchain",
    toolchain_config = ":coral_toolchain_config",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
)

cc_toolchain_config(name = "coral_toolchain_config")

filegroup(
    name = "empty",
    srcs = [],
)