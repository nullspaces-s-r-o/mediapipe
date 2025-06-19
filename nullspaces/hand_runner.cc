#include <memory>
#include <opencv2/opencv.hpp>

#include "../mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker.h"

using namespace mediapipe::tasks::vision;
using namespace cv;

int main() {
  auto options = std::make_unique<hand_landmarker::HandLandmarkerOptions>();
  options->running_mode = core::RunningMode::IMAGE;
  options->base_options.model_asset_path =
      "/home/radxa/ssd/nullspaces/mediapipe/hand_landmarker.task";  // Update with your model path

  auto statusOrLandmarker =
      hand_landmarker::HandLandmarker::Create(std::move(options));

  if (!statusOrLandmarker.ok()) {
    std::cerr << "Error creating HandLandmarker: "
              << statusOrLandmarker.status().message() << std::endl;
    return -1;
  }

  auto landmarker = std::move(statusOrLandmarker.value());
  // Example usage of the landmarker
  // mediapipe::Image image = mediapipe::Image();

  // landmarker->get()->Detect

  // Mat image(640, 480, CV_8UC3, Scalar(0, 0, 0));  // Create a black image
  // Mat image = imread("/home/jiri/google/hand.jpg");
  Mat image;
  cv::VideoCapture cap(0);
  if (!cap.isOpened()) {
    std::cerr << "Error: Could not open camera." << std::endl;
    return -1;
  }

  cv::namedWindow("Hand Landmarker", cv::WINDOW_AUTOSIZE);

  while (true) {
    cap >> image;  // Capture a single frame from the camera

    if (image.empty()) {
      std::cerr << "Error: Could not read frame from camera." << std::endl;
      break;
    }

    mediapipe::Image mpImage(
        mediapipe::ImageFrameSharedPtr(std::make_shared<mediapipe::ImageFrame>(
            mediapipe::ImageFormat::SRGB, image.cols, image.rows, image.step[0],
            image.data, mediapipe::ImageFrame::PixelDataDeleter::kNone)));

    auto result = landmarker->Detect(mpImage);

    if (!result.ok()) {
      std::cerr << "Error detecting hand landmarks: "
                << result.status().message() << std::endl;
      return -1;
    }
    auto handLandmarkerResult = result.value();

    imshow("Hand Landmarker", image);
    if (waitKey(30) >= 0) {
      break;  // Exit on any key press
    }
  }

  return 0;
}