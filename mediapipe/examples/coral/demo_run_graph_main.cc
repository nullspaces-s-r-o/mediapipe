// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// An example of sending OpenCV webcam frames into a MediaPipe graph.
#include <cstdlib>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/opencv_video_inc.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"

#include "rs.hpp"

#define WITH_UI

constexpr char kInputStream[] = "input_video";
constexpr char kOutputStream[] = "output_video";
constexpr char kWindowName[] = "MediaPipe";

ABSL_FLAG(std::string, calculator_graph_config_file, "",
          "Name of file containing text format CalculatorGraphConfig proto.");
ABSL_FLAG(std::string, input_video_path, "",
          "Full path of video to load. "
          "If not provided, attempt to use a webcam.");
ABSL_FLAG(std::string, output_video_path, "",
          "Full path of where to save result (.mp4 only). "
          "If not provided, show result in a window.");

cv::Mat GetMat(rs2::video_frame color){
  return cv::Mat(color.get_height(), color.get_width(), CV_8UC3, (void*)color.get_data() );
}

absl::Status RunMPPGraph() {
  std::string calculator_graph_config_contents;
  MP_RETURN_IF_ERROR(mediapipe::file::GetContents(
      absl::GetFlag(FLAGS_calculator_graph_config_file),
      &calculator_graph_config_contents));
  LOG(INFO) << "Get calculator graph config contents: "
            << calculator_graph_config_contents;
  mediapipe::CalculatorGraphConfig config =
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
          calculator_graph_config_contents);

  LOG(INFO) << "Initialize the calculator graph.";
  mediapipe::CalculatorGraph graph;
  MP_RETURN_IF_ERROR(graph.Initialize(config));

  LOG(INFO) << "Initialize the camera or load the video.";

  rs2::config rs_config;
  rs_config.disable_all_streams();
  rs_config.enable_stream(rs2_stream::RS2_STREAM_COLOR, 1920, 1080,
                      rs2_format::RS2_FORMAT_RGB8);

  rs2::pipeline pipe;
  pipe.start(rs_config);
  auto color_sensor =
      pipe.get_active_profile().get_device().first<rs2::color_sensor>();
  color_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1.0);
  rs2::roi_sensor roiSensor(color_sensor);

  rs2::region_of_interest roi;
  roi.min_x = roi.min_y = 0;
  roi.max_x = 640 - 1;
  roi.max_y = 480 - 1;

  // https://github.com/IntelRealSense/librealsense/issues/8004#issuecomment-746525472
  for (int itrial = 0; itrial < 5; itrial++) {
    try {
      roiSensor.set_region_of_interest(roi);
      break;
    } catch (const rs2::invalid_value_error& ive) {
      std::cerr << ive.what() << std::endl;
    }
  }

  // cv::VideoCapture capture;
  const bool load_video = !absl::GetFlag(FLAGS_input_video_path).empty();
  if (load_video) {
    // capture.open(absl::GetFlag(FLAGS_input_video_path));
  } else {
    // capture.open(cv::CAP_REALSENSE + 1);
    // for(int i = 4; i < 10; i++){
    //   LOG(INFO) << "Attempt to open video " << i;
    //   capture.open(i, cv::CAP_V4L2);
    //   if(capture.isOpened()){
    //     LOG(INFO) << "Camera at index " << i << " initialized successully.";
    //     break;
    //   }
    // }
  }
  // RET_CHECK(capture.isOpened());

  cv::VideoWriter writer;
  const bool save_video = !absl::GetFlag(FLAGS_output_video_path).empty();
  if (save_video) {
    LOG(INFO) << "Prepare video writer.";
    cv::Mat test_frame;
    // capture.read(test_frame);                    // Consume first frame.
    // capture.set(cv::CAP_PROP_POS_AVI_RATIO, 0);  // Rewind to beginning.
    // writer.open(absl::GetFlag(FLAGS_output_video_path),
    //             mediapipe::fourcc('a', 'v', 'c', '1'),  // .mp4
    //             capture.get(cv::CAP_PROP_FPS), test_frame.size());
    RET_CHECK(writer.isOpened());
  } else {
#if defined(WITH_UI)
    cv::namedWindow(kWindowName, /*flags=WINDOW_AUTOSIZE*/ 1);
    // capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    // capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    // capture.set(cv::CAP_PROP_AUTOFOCUS, 0);
    // capture.set(cv::CAP_PROP_FOCUS, 1);
    // capture.set(cv::CAP_PROP_FPS, 30);
#endif
  }

  // capture >> tmp;

  cv::Mat tmp = GetMat(pipe.wait_for_frames().get_color_frame());
  LOG(INFO) << "Image size is " << tmp.cols << "x" << tmp.rows;
  
  LOG(INFO) << "Start running the calculator graph.";
  ASSIGN_OR_RETURN(mediapipe::OutputStreamPoller poller,
                   graph.AddOutputStreamPoller(kOutputStream));
  MP_RETURN_IF_ERROR(graph.StartRun({}));

  LOG(INFO) << "Start grabbing and processing frames.";

  cv::Rect src_roi;
  src_roi.height = tmp.rows;
  src_roi.width = 4 * tmp.rows / 3;
  src_roi.y = 0;
  src_roi.x = (tmp.cols - src_roi.width) / 2;

  LOG(INFO) << "src_roi: " << src_roi.x << ", " << src_roi.y << " - " << src_roi.width << "x" << src_roi.height;

  double s = 640 / (double)src_roi.width;
  double t = src_roi.x;
  cv::Mat S = s * cv::Mat_<double>::eye(3, 3);
  cv::Mat T = (cv::Mat_<double>(3, 3) <<
    1.0, 0.0, -t,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0);

  cv::Mat A = cv::Mat(S * T).rowRange(0, 2);

  bool grab_frames = true;
  while (grab_frames) {
    // Capture opencv camera or video frame.
    auto color = pipe.wait_for_frames(-1).get_color_frame();
    cv::Mat camera_frame_raw = GetMat(color);
    // capture >> camera_frame_raw;
    // if (camera_frame_raw.empty()) break;  // End of video.
    static cv::Mat camera_frame; // = camera_frame_raw;
    // cv::resize(camera_frame_raw(src_roi), camera_frame, { 640, 480});
    // static cv::Mat crop;
    cv::warpAffine(camera_frame_raw, camera_frame, A, { 640, 480}, cv::INTER_NEAREST);
    // camera_frame = camera_frame_raw;
    // cv::cvtColor(crop, camera_frame, cv::COLOR_YUV2BGR_YUYV);
    // if (!load_video) {
    //   cv::flip(camera_frame, camera_frame, /*flipcode=HORIZONTAL*/ 1);
    // }

    // Wrap Mat into an ImageFrame.
    auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
        mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
        mediapipe::ImageFrame::kDefaultAlignmentBoundary);
    cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
    camera_frame.copyTo(input_frame_mat);

    // Send image packet into the graph.
    size_t frame_timestamp_us =
        (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
    MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
        kInputStream, mediapipe::Adopt(input_frame.release())
                          .At(mediapipe::Timestamp(frame_timestamp_us))));

    // Get the graph result packet, or stop if that fails.
    mediapipe::Packet packet;
    if (!poller.Next(&packet)) break;
    auto& output_frame = packet.Get<mediapipe::ImageFrame>();

    // Convert back to opencv for display or saving.
    cv::Mat output_frame_mat = mediapipe::formats::MatView(&output_frame);
    static cv::Mat preview;
    cv::cvtColor(output_frame_mat, preview, cv::COLOR_RGB2BGR);
    if (save_video) {
      writer.write(output_frame_mat);
    } else {
#if defined(WITH_UI)

      cv::imshow(kWindowName, preview);
      // Press any key to exit.
      const int pressed_key = cv::waitKey(5);
      if (pressed_key == 27) grab_frames = false;
#endif
    }
  }

  pipe.stop();

  LOG(INFO) << "Shutting down.";
  if (writer.isOpened()) writer.release();
  MP_RETURN_IF_ERROR(graph.CloseInputStream(kInputStream));
  return graph.WaitUntilDone();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  absl::Status run_status = RunMPPGraph();
  if (!run_status.ok()) {
    LOG(ERROR) << "Failed to run the graph: " << run_status.message();
    return EXIT_FAILURE;
  } else {
    LOG(INFO) << "Success!";
  }
  return EXIT_SUCCESS;
}
