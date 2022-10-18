#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "mediapipe/framework/calculator_framework.h"
// #include "tensorflow/lite/c/common.h" // TFliteTensor
#include "mediapipe/framework/formats/tensor.h"

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

namespace mediapipe {

class InferenceCaptureCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc){
    RET_CHECK(cc->Inputs().HasTag(kNetInputTag));
    RET_CHECK(cc->Inputs().HasTag(kNetOutputTag));

    cc->Inputs().Tag(kNetInputTag).Set<std::vector<mediapipe::Tensor>>();
    cc->Inputs().Tag(kNetOutputTag).Set<std::vector<mediapipe::Tensor>>();

    return absl::OkStatus();
  }

  virtual absl::Status Open(CalculatorContext* cc) override {
    return absl::OkStatus();
  }

  virtual absl::Status Process(CalculatorContext* cc) override {
    return absl::OkStatus();
  }

  virtual absl::Status Close(CalculatorContext* cc) override {
    return absl::OkStatus();
  }
};

REGISTER_CALCULATOR(InferenceCaptureCalculator);

}  // namespace mediapipe