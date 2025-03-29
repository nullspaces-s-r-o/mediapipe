cc_library(
    name = "realsense2",
    hdrs = glob([
        "include/librealsense2/**/*.h*"
    ]),
    includes = [
        "include/librealsense2"
    ],
    linkopts = [
        "-l:librealsense2.so",
        "-l:libz.so",
        # "-l:libusb-1.0.so",
        "-L/home/shared/coral/sysroot/usr/lib/aarch64-linux-gnu",
        #"-l:libudev.so",
        # "-L/home/shared/coral/sysroot/usr/local/lib/",
        # "-L/home/shared/coral/sysroot/lib/aarch64-linux-gnu",
    ],
    visibility = ["//visibility:public"],
)