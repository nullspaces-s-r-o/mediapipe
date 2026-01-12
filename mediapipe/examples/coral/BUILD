# Copyright 2019 The MediaPipe Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])

package(default_visibility = [
    "//visibility:public",
])

# Graph Runner

cc_library(
    name = "demo_run_graph_main",
    srcs = ["demo_run_graph_main.cc"],
    deps = [
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/framework/formats:image_frame_opencv",
        "//mediapipe/framework/port:file_helpers",
        "//mediapipe/framework/port:opencv_highgui",
        "//mediapipe/framework/port:opencv_imgproc",
        "//mediapipe/framework/port:opencv_video",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/port:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/log:absl_log",
    ],
)

# Demos

cc_binary(
    name = "object_detection_tpu",
    deps = [
        "//mediapipe/calculators/image:image_transformation_calculator",
        "//mediapipe/examples/coral:demo_run_graph_main",
        "//mediapipe/graphs/object_detection:desktop_tflite_calculators",
    ],
)

cc_binary(
    name = "face_detection_tpu",
    deps = [
        "//mediapipe/calculators/image:image_transformation_calculator",
        "//mediapipe/calculators/tflite:tflite_converter_calculator",
        "//mediapipe/calculators/tflite:tflite_inference_calculator",
        "//mediapipe/calculators/tflite:tflite_tensors_to_detections_calculator",
        "//mediapipe/calculators/util:detection_label_id_to_text_calculator",
        "//mediapipe/calculators/util:detection_letterbox_removal_calculator",
        "//mediapipe/examples/coral:demo_run_graph_main",
        "//mediapipe/graphs/face_detection:desktop_live_calculators",
    ],
)

# Build the shared library
cc_binary(
    name = "libhand_tracking_tpu_lib.so",
    srcs = [
        "hand_tracking_tpu_lib.cc", 
        "hand_tracking_tpu_lib.h"
    ],
    copts = [ 
        "-DCOMPILING_HAND_TRACKING_TPU_LIB",
        "-fvisibility=hidden", 
        "-fvisibility-inlines-hidden", 
        ], 
    linkopts = [ 
        "-Wl,--exclude-libs,ALL",
    ],
    deps = [
        "//mediapipe/calculators/core:constant_side_packet_calculator",
        "//mediapipe/calculators/core:constant_side_packet_calculator_cc_proto",
        "//mediapipe/modules/hand_landmark:hand_landmark_tracking_cpu",
        "//mediapipe/calculators/tensor:image_to_image_calculator",
        # run_graph_main
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/framework/formats:image_frame_opencv",
        "//mediapipe/framework/port:file_helpers",
        "//mediapipe/framework/port:opencv_highgui",
        "//mediapipe/framework/port:opencv_imgproc",
        "//mediapipe/framework/port:opencv_video",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/port:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/log:absl_log",

        # hand renderer
        "//mediapipe/graphs/hand_tracking/subgraphs:hand_renderer_cpu",

    ],
    linkshared = True, # Build as .so, important
    linkstatic = True, # Statically link dependencies
)

# Import the shared library
cc_import(
    name = "imported_hand_tracking_tpu_lib",
    hdrs = ["hand_tracking_tpu_lib.h"],
    shared_library = "libhand_tracking_tpu_lib.so",
)

# cc_binary(
#     name = "hand_tracking_tpu_lib.so",
#     srcs = [
#         "hand_tracking_tpu_lib.cc",
#     ],
#     deps = [":hand_tracking_tpu_lib"],
#     linkshared = True,
#     linkstatic = True,  # <-- Statically link all deps into the .so
#     visibility = ["//visibility:public"],
#     copts = [
#         "-DCOMPILING_HAND_TRACKING_TPU_LIB"
#     ]
# )

cc_binary(
    name = "hand_tracking_tpu_client",
    srcs = ["hand_tracking_tpu_client.cc"],
    deps = [
        ":imported_hand_tracking_tpu_lib",
        "//mediapipe/framework/formats:image_frame_opencv",
        "//mediapipe/framework/port:opencv_highgui",
        "//mediapipe/framework/port:opencv_imgproc",
        "//mediapipe/framework/port:opencv_video",
    ],
    # linkopts = [
    # "-L/home/jiri/coral/buildsys/build/gperftools",  
    # "-ltcmalloc"],
    linkstatic = True
)

cc_binary(
    name = "hand_tracking_tpu",
    deps = [
        ":imported_hand_tracking_tpu_lib",
        # "//mediapipe/calculators/core:constant_side_packet_calculator",
        # "//mediapipe/calculators/core:constant_side_packet_calculator_cc_proto",
        # "//mediapipe/modules/hand_landmark:hand_landmark_tracking_cpu",
        # "//mediapipe/calculators/tensor:image_to_image_calculator",
        
        # app
        "//mediapipe/graphs/hand_tracking/subgraphs:hand_renderer_cpu",
        "//mediapipe/examples/coral:demo_run_graph_main",



    ],
    # linkstatic = True # Dynamically link to hand_tracking_tpu_lib.so
)

# bazel build 
#     --config=dad_config 
#     --cpu=aarch64 
#     --compiler=coral-gcc 
#     --copt=-DLIBYUV_DISABLE_NEON 
#     --define MEDIAPIPE_DISABLE_GPU=1 
#     --define MEDIAPIPE_EDGE_TPU=pci 
#     --linkopt=-l:libusb-1.0.so 
#     --define darwinn_portable=1 
#     mediapipe/examples/coral:hand_tracking_tpu
