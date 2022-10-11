cc_library(
    name = "network_camera",
    hdrs = [
        "NetworkCamera.h",
        "ImageProvider.h"
    ],
    includes = [
        "/home/jiri/DigitalAssistant/headless-calibration/ImageProviders"
    ],
    linkopts = [
        "-l:libimage_providers_lib.a",
        "-L/home/jiri/DigitalAssistant/build/image-providers",
    ],
    visibility = ["//visibility:public"],
)