load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

all_link_actions = [ 
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [ 
        tool_path(
            name = "gcc",
            path = "/home/jiri/coral/x-tools/aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc",
        ),
        tool_path(
            name = "ld",
            path = "/home/jiri/coral/x-tools/aarch64-linux-gnu/bin/aarch64-linux-gnu-ld",
        ),
        tool_path(
            name = "ar",
            path = "/home/jiri/coral/x-tools/aarch64-linux-gnu/bin/aarch64-linux-gnu-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/home/jiri/coral/x-tools/aarch64-linux-gnu/bin/aarch64-linux-gnu-cpp",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/home/jiri/coral/x-tools/aarch64-linux-gnu/bin/aarch64-linux-gnu-nm",
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/home/jiri/coral/x-tools/aarch64-linux-gnu/bin/aarch64-linux-gnu-strip",
        ),
    ]

    features = [
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-lstdc++",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [ # NEW
            "/home/jiri/coral/sysroot/usr/include",
            "/home/jiri/coral/sysroot/usr/local/include",
            "/home/jiri/coral/x-tools/aarch64-linux-gnu/aarch64-linux-gnu/sysroot/usr/include",
            "/home/jiri/coral/x-tools/aarch64-linux-gnu/aarch64-linux-gnu/include",
            "/home/jiri/coral/x-tools/aarch64-linux-gnu/include",
            "/home/jiri/coral/x-tools/aarch64-linux-gnu/lib/gcc/aarch64-linux-gnu/8.5.0/include",
        ],
        toolchain_identifier = "aarch64-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "aarch64",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)