// #include <iostream>
// #include <glog/logging.h>

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

// constexpr char kInputStream[] = "input_video";
// constexpr char kOutputStream[] = "output_video";
// constexpr char kWindowName[] = "MediaPipe";

// ABSL_FLAG(std::string, calculator_graph_config_file, "",
//           "Name of file containing text format CalculatorGraphConfig proto.");
// ABSL_FLAG(std::string, input_video_path, "",
//           "Full path of video to load. "
//           "If not provided, attempt to use a webcam.");
// ABSL_FLAG(std::string, output_video_path, "",
//           "Full path of where to save result (.mp4 only). "
//           "If not provided, show result in a window.");

// mediapipe::CalculatorGraph graph;

// absl::Status GraphInit()
// {
//     std::string calculator_graph_config_contents;
//     MP_RETURN_IF_ERROR(mediapipe::file::GetContents(
//         absl::GetFlag(FLAGS_calculator_graph_config_file),
//         &calculator_graph_config_contents));
//     ABSL_LOG(INFO) << "Get calculator graph config contents: "
//                    << calculator_graph_config_contents;
//     mediapipe::CalculatorGraphConfig config =
//         mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
//             calculator_graph_config_contents);

//     ABSL_LOG(INFO) << "Initialize the calculator graph.";
//     MP_RETURN_IF_ERROR(graph.Initialize(config));

//     return graph.StartRun({});
// }

// absl::Status DestroyGraph()
// {
//     return graph.WaitUntilDone();
// }

// absl::Status GraphAcceptCameraFrame(const cv::Mat &camera_frame)
// {
//     // Wrap Mat into an ImageFrame.
//     auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
//         mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
//         mediapipe::ImageFrame::kDefaultAlignmentBoundary);
//     cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
//     camera_frame.copyTo(input_frame_mat);

//     // Send image packet into the graph.
//     size_t frame_timestamp_us =
//         (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
//     MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
//         kInputStream, mediapipe::Adopt(input_frame.release())
//                           .At(mediapipe::Timestamp(frame_timestamp_us))));

//     return absl::OkStatus();
// }

#include "mediapipe/framework/formats/image_frame_opencv.h"

#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/opencv_video_inc.h"

#include "hand_tracking_tpu_lib.h"

int main(int argc, char **argv)
{
    // google::InitGoogleLogging(argv[0]);

    GraphInit(argv[1]);

    cv::VideoCapture cap(25);

    cv::Mat camera_frame(640, 480, CV_8UC3);

    while (true)
    {
        cap >> camera_frame;

        cv::imshow("input_frame", camera_frame);
        cv::waitKey(1);

        cv::cvtColor(camera_frame, camera_frame, cv::COLOR_BGR2RGB);

        GraphAcceptCameraFrame(camera_frame);
        const cv::Mat &output_frame = GetOutputFrame();

        if (output_frame.empty())
        {
            std::cout << "Output frame is empty." << std::endl;
            continue;
        }

        cv::imshow("MediaPipe", output_frame);
        if (cv::waitKey(5) >= 0)
            break;
    }

    GraphDestroy();
    // absl::ParseCommandLine(argc, argv);
    // absl::Status run_status = GraphInit();
    // if (!run_status.ok())
    // {
    //     ABSL_LOG(ERROR) << "Failed to run the graph: " << run_status.message();
    //     return EXIT_FAILURE;
    // }

    // else
    // {
    //     ABSL_LOG(INFO) << "Success!";

    //     cv::Mat camera_frame(640, 480, CV_8UC3);
    //     GraphAcceptCameraFrame(camera_frame);
    // }

    // DestroyGraph();

    // return EXIT_SUCCESS;

    return 0;
}