load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")

load("@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl", 
    "feature",   
    "flag_group",
    "flag_set",  
    "tool_path"
)

all_link_actions = [ 
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",  # Compiler is referenced by the name "gcc" for historic reasons.
            path = "/home/jiri/rockp/aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc",
        ),
        tool_path(
            name = "ld",
            path = "/home/jiri/rockp/aarch64-linux-gnu/bin/aarch64-linux-gnu-ld",
        ),
        tool_path(
            name = "ar",
            path = "/home/jiri/rockp/aarch64-linux-gnu/bin/aarch64-linux-gnu-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/bin/false",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/bin/false",
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/bin/false",
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
        cxx_builtin_include_directories = [ 
            "/home/jiri/rockp/aarch64-linux-gnu/aarch64-linux-gnu/include",
            "/home/jiri/rockp/aarch64-linux-gnu/aarch64-linux-gnu/sysroot/usr/include",
            "/home/jiri/rockp/aarch64-linux-gnu/aarch64-linux-gnu/include",
            "/home/jiri/rockp/aarch64-linux-gnu/aarch64-linux-gnu/sysroot/usr/include",
            "/home/jiri/rockp/aarch64-linux-gnu/lib/gcc/aarch64-linux-gnu/11.4.0/include",
            "/home/jiri/rockp/aarch64-linux-gnu/lib/gcc/aarch64-linux-gnu/11.4.0/include-fixed",
        ],
        toolchain_identifier = "rock-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "aarch64",
        target_libc = "unknown",
        compiler = "gcc",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)