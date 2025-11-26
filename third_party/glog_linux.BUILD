licenses(["notice"])  # BSD or Apache, as appropriate

exports_files(["LICENSE"])

cc_library(
    name = "glog",
    hdrs = glob([
        "include/glog/*.h",
    ]),
    includes = [
        "include",
    ],
    linkopts = [
        "-Llib",
        "-lglog",
    ],
    copts = [
        "-DGLOG_STATIC_DEFINE",
        "-DGLOG_USE_GLOG_EXPORT"
    ],
    visibility = ["//visibility:public"],
)