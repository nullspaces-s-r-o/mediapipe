cc_library(
    name = "network_camera",
    hdrs = [
        "NetworkCamera.h",
        "ImageProvider.h"
    ],
    includes = [
        "/home/shared/DigitalAssistant/headless-calibration/ImageProviders"
    ],
    linkopts = [
        "-l:libimage_providers_lib.a",
        "-L/home/shared/DigitalAssistant/build/image-providers",
    ],
    visibility = ["//visibility:public"],
)