#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "mediapipe/framework/calculator_framework.h"
// #include "tensorflow/lite/c/common.h" // TFliteTensor
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "mediapipe/framework/formats/tensor.h"
#include "mediapipe/graphs/hand_tracking/calculators/inference_capture_calculator.pb.h"

// file io
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
Input:
  TENSORS - image tensor
  TENSORS - interpreter output tensors
Output:
  <no output>

node {
  calculator: "InferenceCaptureCalculator"
  input_stream: "TENSORS:0:input_tensor"
  input_stream: "TENSORS:1:detection_tensors"
  # options: {
  #   [mediapipe.InferenceCalculatorOptions.ext] {
  #     delegate { xnnpack {} }
  #   }
  # }
}
*/

constexpr char kNetInputTag[] = "TENSORS_NETINPUT";
constexpr char kNetOutputTag[] = "TENSORS_NETOUTPUT";

constexpr char kTimeFormat[] = "%Y_%m_%d-%H_%M_%E*S";

typedef std::vector<mediapipe::Tensor> data_type;

namespace mediapipe {

class InferenceCaptureCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(cc->Inputs().HasTag(kNetInputTag));
    RET_CHECK(cc->Inputs().HasTag(kNetOutputTag));

    cc->Inputs().Tag(kNetInputTag).Set<data_type>();
    cc->Inputs().Tag(kNetOutputTag).Set<data_type>();

    return absl::OkStatus();
  }

  virtual absl::Status Open(CalculatorContext* cc) override {
    // Build full image path
    const auto& options = cc->Options<InferenceCaptureCalculatorOptions>();

    // Check and/or create directory
    struct stat sb;
    if (lstat(options.export_path().c_str(), &sb) == -1) {
      mkdir(options.export_path().c_str(), S_IRWXU);
    }

    return absl::OkStatus();
  }

  virtual absl::Status Process(CalculatorContext* cc) override {
    const auto& options = cc->Options<InferenceCaptureCalculatorOptions>();
    std::string filePath = options.export_path() + "/" + timestamp();

    saveNetInput(cc, filePath);
    saveNetOutput(cc, filePath);

    return absl::OkStatus();
  }

  virtual absl::Status Close(CalculatorContext* cc) override {
    return absl::OkStatus();
  }

 private:
  std::string timestamp() {
    absl::Time t1 = absl::Now();
    absl::TimeZone utc = absl::UTCTimeZone();

    std::stringstream ss;
    ss << absl::FormatTime(kTimeFormat, t1, utc);

    std::string str = ss.str();
    std::replace(begin(str), end(str), '.', '_');

    return str;
  }

  absl::Status saveNetInput(CalculatorContext* cc,
                            const std::string& filePath) {
    const auto& input = cc->Inputs().Tag(kNetInputTag);
    const data_type& net_input = input.Get<data_type>();

    const Tensor& tensor = net_input[0];
    const Tensor::Shape& shape = tensor.shape();
    const mediapipe::Tensor::CpuReadView& view = tensor.GetCpuReadView();
    const float* buffer = view.buffer<float>();

    // Mat to normalized <0, 1> float tensor
    cv::Mat flt_rgb(shape.dims[1], shape.dims[2], CV_32FC3, (void*)buffer);

    // Conver to integer <0, 255> uint8 image
    cv::Mat int_rgb, int_bgr;
    flt_rgb.convertTo(int_rgb, CV_8UC3, 255.0, 0.0);

    // Convert color from RGB to BGR (as opencv expects)
    cv::cvtColor(int_rgb, int_bgr, cv::COLOR_RGB2BGR);

    // Write to disk
    const std::string fullPath = filePath + "_palm.png";
    if (!cv::imwrite(fullPath, int_bgr)) {
      return absl::AbortedError("Failed to write image " + fullPath + ".");
    }

    return absl::OkStatus();
  }

  absl::Status saveNetOutput(CalculatorContext* cc,
                             const std::string& filePath) {
    const auto& output = cc->Inputs().Tag(kNetOutputTag);
    const data_type& net_output = output.Get<data_type>();

    const std::string fullPath = filePath + "_label.json";
    cv::FileStorage fs(fullPath, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
      return absl::AbortedError("Failed to open " + fullPath + " for writing.");
    }

    fs << "out_tensors";
    fs << "[";
    for (const Tensor& tensor : net_output) {
      fs << "{";
      const Tensor::Shape& shape = tensor.shape();
      fs << "shape";
      fs << "[";
      for(const int& dim : shape.dims){
        fs << dim;
      }

      fs << "]";  // shape

      const mediapipe::Tensor::CpuReadView& view = tensor.GetCpuReadView();
      const float* buffer = view.buffer<float>();

      int numValues = tensor.bytes() / tensor.element_size();

      fs << "values";
      fs << "[";
      for (size_t i = 0; i < numValues; i++) {
        fs << buffer[i];
      }
      fs << "]"; // values
      fs << "}"; // tensor
    }
    fs << "]";  // out_tensors

    return absl::OkStatus();
  }
};

REGISTER_CALCULATOR(InferenceCaptureCalculator);

}  // namespace mediapipe