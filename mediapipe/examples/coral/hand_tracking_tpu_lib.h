// #include <iostream>
// #include <glog/logging.h>

// #include "absl/status/status.h"
// #include "absl/flags/flag.h"
// #include "absl/flags/parse.h"
// #include "absl/log/absl_log.h"
// #include "mediapipe/framework/calculator_framework.h"
// #include "mediapipe/framework/formats/image_frame.h"
// #include "mediapipe/framework/formats/image_frame_opencv.h"
// #include "mediapipe/framework/port/file_helpers.h"
// #include "mediapipe/framework/port/opencv_highgui_inc.h"
// #include "mediapipe/framework/port/opencv_imgproc_inc.h"
// #include "mediapipe/framework/port/opencv_video_inc.h"
// #include "mediapipe/framework/port/parse_text_proto.h"
// #include "mediapipe/framework/port/status.h"

namespace cv
{
    class Mat;
}

#if defined(_MSC_VER)
//  Microsoft
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#define EXPORT __attribute__((visibility("default")))
#define IMPORT
#else
//  do nothing and hope for the best?
#define EXPORT
#define IMPORT
#pragma warning Unknown dynamic link import / export semantics.
#endif

#if COMPILING_HAND_TRACKING_TPU_LIB
#define MY_LIB_PUBLIC EXPORT
#else
#define MY_LIB_PUBLIC IMPORT
#endif

extern "C"
{
    MY_LIB_PUBLIC int GraphInit(const char *config_file);
    MY_LIB_PUBLIC int GraphDestroy();
    MY_LIB_PUBLIC int GraphAcceptCameraFrame(const cv::Mat &camera_frame);
    MY_LIB_PUBLIC const cv::Mat &GetOutputFrame();
}