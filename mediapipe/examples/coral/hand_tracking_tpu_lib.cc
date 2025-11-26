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
#include "mediapipe/framework/formats/landmark.pb.h"
#include <memory>

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

mediapipe::CalculatorGraph *graph = nullptr;
std::unique_ptr<mediapipe::OutputStreamPoller> ppoller;
std::unique_ptr<mediapipe::OutputStreamPoller> landmarks_poller;

extern "C"
{

    int GraphInit(const char *config_file)
    {
        if (graph != nullptr)
        {
            LOG(ERROR) << "Graph already initialized";
            return -1;
        }

        std::string calculator_graph_config_contents;
        (mediapipe::file::GetContents(config_file, &calculator_graph_config_contents));
        mediapipe::CalculatorGraphConfig config =
            mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
                calculator_graph_config_contents);

        graph = new mediapipe::CalculatorGraph();
        absl::Status status = graph->Initialize(config);
        ABSL_LOG(INFO) << "Graph initialization status: " << status;
        if (!status.ok())
        {
            delete graph;
            graph = nullptr;
            return -1;
        }

        // Require the output_video poller to exist — otherwise return error so you
        // don't keep pushing frames into a graph with no consumer.
        auto status_or_poller = graph->AddOutputStreamPoller(kOutputStream);
        if (!status_or_poller.ok())
        {
            ABSL_LOG(ERROR) << "No output stream '" << kOutputStream << "' in graph: "
                            << status_or_poller.status();
            delete graph;
            graph = nullptr;
            return -1;
        }
        ppoller = std::make_unique<mediapipe::OutputStreamPoller>(std::move(status_or_poller.value()));

        // Also create a poller for the landmarks stream so those packets are consumed
        // (prevents calculators from buffering results internally).
        auto land_or_poller = graph->AddOutputStreamPoller("landmarks");
        if (land_or_poller.ok())
        {
            landmarks_poller = std::make_unique<mediapipe::OutputStreamPoller>(std::move(land_or_poller.value()));
        }
        else
        {
            ABSL_LOG(INFO) << "No 'landmarks' output stream available: " << land_or_poller.status();
            // not fatal — but if your graph produces landmarks you should poll them
        }

        auto run_status = graph->StartRun({});

        LOG(INFO) << "Graph run status: " << run_status;

        return run_status == absl::OkStatus() ? 0 : -1;
    }

    int GraphDestroy()
    {
        if (!graph)
            return -1;
        graph->CloseInputStream(kInputStream);
        absl::Status status = graph->WaitUntilDone();
        // Free resources
        ppoller.reset();
        delete graph;
        graph = nullptr;
        return status == absl::OkStatus() ? 0 : -1;
    }

    int GraphAcceptCameraFrame(const cv::Mat &camera_frame)
    {
        if (!graph)
            return -1;

        // Wrap Mat into an ImageFrame.
        auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
            mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
            mediapipe::ImageFrame::kDefaultAlignmentBoundary);
        cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
        camera_frame.copyTo(input_frame_mat);

        // Send image packet into the graph.
        size_t frame_timestamp_us =
            (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;

        // Create a Packet that owns the released ImageFrame pointer. If AddPacketToInputStream
        // fails, 'packet' will be destroyed and will free the ImageFrame — avoiding leaks.
        mediapipe::Packet packet = mediapipe::Adopt(input_frame.release())
                                       .At(mediapipe::Timestamp(frame_timestamp_us));
        absl::Status add_status = graph->AddPacketToInputStream(kInputStream, std::move(packet));
        if (!add_status.ok())
        {
            ABSL_LOG(WARNING) << "Failed to add packet to input stream: " << add_status;
            return -1;
        }

        return 0;
    }

    const cv::Mat GetOutputFrame()
    {
        cv::Mat output_frame_mat;

        // Get the graph result packet, or stop if that fails.
        mediapipe::Packet packet;
        if (ppoller)
        {
            if (ppoller->Next(&packet))
            {
                auto &output_frame = packet.Get<mediapipe::ImageFrame>();
                // Clone so cv::Mat owns its memory independently of the packet/ImageFrame.
                output_frame_mat = mediapipe::formats::MatView(&output_frame).clone();
                // cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGB2BGR);
            }
        }

        // Convert back to opencv for display or saving.
        return output_frame_mat;
    }

    // Example: Get landmarks as a vector of floats
    int GetLandmarks(std::vector<NormalizedLandmarkList> &out_landmarks)
    {
        out_landmarks.clear();

        if (!landmarks_poller)
            return -1;

        // Only try to get a packet if one is available
        if (landmarks_poller->QueueSize() > 0)
        {
            mediapipe::Packet packet;
            if (landmarks_poller->Next(&packet))
            {
                // Adjust the type below to match your graph's output type.
                auto out_landmarks_mp = packet.Get<std::vector<mediapipe::NormalizedLandmarkList>>();

                for (const auto &landmark_list_mp : out_landmarks_mp)
                {
                    NormalizedLandmarkList landmark_list;
                    for (int i = 0; i < landmark_list_mp.landmark_size(); ++i)
                    {
                        const auto &landmark_mp = landmark_list_mp.landmark(i);
                        Landmark2 landmark;
                        landmark.x = landmark_mp.x();
                        landmark.y = landmark_mp.y();
                        landmark.z = landmark_mp.z();
                        landmark.visibility = landmark_mp.visibility();
                        landmark.presence = landmark_mp.presence();
                        landmark_list.landmark.push_back(landmark);
                    }
                    out_landmarks.push_back(landmark_list);
                }

                return out_landmarks_mp.size();
            }
        }
        return -1;
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