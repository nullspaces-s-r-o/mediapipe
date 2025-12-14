#ifndef MEDIAPIPE_EXAMPLES_CORAL_HAND_TRACKING_TPU_LIB_H_
#define MEDIAPIPE_EXAMPLES_CORAL_HAND_TRACKING_TPU_LIB_H_

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

struct Landmark2
{
    float x;
    float y;
    float z;
    float visibility;
    float presence;
};

struct NormalizedLandmarkList
{
    std::vector<Landmark2> landmark;
    std::vector<Landmark2> world_landmark;
};

extern "C"
{
    MY_LIB_PUBLIC int GraphInit(const char *config_file);
    MY_LIB_PUBLIC int GraphDestroy();
    MY_LIB_PUBLIC int GraphAcceptCameraFrame(const cv::Mat &camera_frame);
    MY_LIB_PUBLIC const cv::Mat GetOutputFrame();
    MY_LIB_PUBLIC int GetLandmarks(std::vector<NormalizedLandmarkList> &out_landmarks);
    MY_LIB_PUBLIC int GetWorldLandmarks(std::vector<NormalizedLandmarkList> &out_landmarks);
    MY_LIB_PUBLIC int GetImageAndWorldLandmarks(std::vector<NormalizedLandmarkList> &out_landmarks);
}

#endif // MEDIAPIPE_EXAMPLES_CORAL_HAND_TRACKING_TPU_LIB_H_