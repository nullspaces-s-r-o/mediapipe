cc_library(
    name = "imagestreamer",
    hdrs = [
        "application/diag-plugin/util/ImageStreamer.h",
        "diagnostic/remote_debugger_server/helper.h",
        "build/grpc/image.pb.h",
    ],
    includes = [
        "application/diag-plugin/util",
        "diagnostic/remote_debugger_server",
        "build/grpc",
    ],
    linkopts = [
        "-l:libimagestreamer.so",
    ],
    visibility = ["//visibility:public"],
)

# bazel build --verbose_failures --define MEDIAPIPE_DISABLE_GPU=1 mediapipe/examples/desktop/hand_tracking:hand_tracking_net