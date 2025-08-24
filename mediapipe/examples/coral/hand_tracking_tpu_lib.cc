
#include <iostream>
#include <glog/logging.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/absl_log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/opencv_video_inc.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/graph_output_stream.h" // poller
#include "mediapipe/framework/output_stream_poller.h"

#include "hand_tracking_tpu_lib.h"
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

mediapipe::CalculatorGraph graph;
mediapipe::OutputStreamPoller *ppoller = nullptr;

extern "C"
{

    int GraphInit(const char *config_file)
    {
        std::string calculator_graph_config_contents;
        /*MP_RETURN_IF_ERROR*/ (mediapipe::file::GetContents(
            config_file,
            &calculator_graph_config_contents));
        ABSL_LOG(INFO) << "Get calculator graph config contents: "
                       << calculator_graph_config_contents;
        mediapipe::CalculatorGraphConfig config =
            mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
                calculator_graph_config_contents);

        ABSL_LOG(INFO) << "Initialize the calculator graph.";
        absl::Status status = graph.Initialize(config);
        ABSL_LOG(INFO) << "Graph initialization status: " << status;

        graph.AddOutputStreamPoller(kOutputStream);
        auto status_or_poller =
            graph.AddOutputStreamPoller(kOutputStream);
        // OutputStreamPoller poller = std::move(status_or_poller.value());
        ppoller = new mediapipe::OutputStreamPoller(std::move(status_or_poller.value()));

        return graph.StartRun({}) == absl::OkStatus() ? 0 : -1;
    }

    int GraphDestroy()
    {
        graph.CloseInputStream(kInputStream);
        return graph.WaitUntilDone() == absl::OkStatus() ? 0 : -1;
    }

    int GraphAcceptCameraFrame(const cv::Mat &camera_frame)
    {
        // Wrap Mat into an ImageFrame.
        auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
            mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
            mediapipe::ImageFrame::kDefaultAlignmentBoundary);
        cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
        camera_frame.copyTo(input_frame_mat);

        // Send image packet into the graph.
        size_t frame_timestamp_us =
            (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
        /*MP_RETURN_IF_ERROR*/ (graph.AddPacketToInputStream(
            kInputStream, mediapipe::Adopt(input_frame.release())
                              .At(mediapipe::Timestamp(frame_timestamp_us))));

        return 0;
    }

    const cv::Mat &GetOutputFrame()
    {
        static cv::Mat output_frame_mat;

        // Get the graph result packet, or stop if that fails.
        mediapipe::Packet packet;
        if (ppoller)
        {
            if (ppoller->Next(&packet))
            {
                auto &output_frame = packet.Get<mediapipe::ImageFrame>();
                output_frame_mat = mediapipe::formats::MatView(&output_frame);
                cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGB2BGR);
            }
        }

        // Convert back to opencv for display or saving.
        return output_frame_mat;
    }

} // extern "C"

// int main(int argc, char **argv)
// {
//     google::InitGoogleLogging(argv[0]);
//     absl::ParseCommandLine(argc, argv);
//     absl::Status run_status = GraphInit();
//     if (!run_status.ok())
//     {
//         ABSL_LOG(ERROR) << "Failed to run the graph: " << run_status.message();
//         return EXIT_FAILURE;
//     }

//     else
//     {
//         ABSL_LOG(INFO) << "Success!";

//         cv::Mat camera_frame(640, 480, CV_8UC3);
//         GraphAcceptCameraFrame(camera_frame);
//     }

//     GraphDestroy();

//     return EXIT_SUCCESS;
// }