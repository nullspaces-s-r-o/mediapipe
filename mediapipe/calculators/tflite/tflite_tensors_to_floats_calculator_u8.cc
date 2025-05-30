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
#define DQ(x) (scale * ((float)x - zero_point))

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "tensorflow/lite/interpreter.h"

namespace mediapipe {

constexpr char kFloatsTag[] = "FLOATS";
constexpr char kFloatTag[] = "FLOAT";
constexpr char kTensorsTag[] = "TENSORS";

// A calculator for converting TFLite tensors to to a float or a float vector.
//
// Input:
//  TENSORS - Vector of TfLiteTensor of type kTfLiteFloat32. Only the first
//            tensor will be used.
// Output:
//  FLOAT(optional) - Converted single float number.
//  FLOATS(optional) - Converted float vector.
//
// Notes: To output FLOAT stream, the input TFLite tensor must have size 1, e.g.
//        only 1 float number in the tensor.
//
// Usage example:
// node {
//   calculator: "TfLiteTensorsToFloatsCalculator"
//   input_stream: "TENSORS:tensors"
//   output_stream: "FLOATS:floats"
// }
class TfLiteTensorsToFloatsCalculatorU8 : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc) override;

  absl::Status Process(CalculatorContext* cc) override;
};
REGISTER_CALCULATOR(TfLiteTensorsToFloatsCalculatorU8);

absl::Status TfLiteTensorsToFloatsCalculatorU8::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kTensorsTag));
  RET_CHECK(cc->Outputs().HasTag(kFloatsTag) ||
            cc->Outputs().HasTag(kFloatTag));

  cc->Inputs().Tag(kTensorsTag).Set<std::vector<TfLiteTensor>>();
  if (cc->Outputs().HasTag(kFloatsTag)) {
    cc->Outputs().Tag(kFloatsTag).Set<std::vector<float>>();
  }
  if (cc->Outputs().HasTag(kFloatTag)) {
    cc->Outputs().Tag(kFloatTag).Set<float>();
  }

  return absl::OkStatus();
}

absl::Status TfLiteTensorsToFloatsCalculatorU8::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  return absl::OkStatus();
}

void GetQuantizationParams(const TfLiteTensor *tensor, float &scale,
                                      float &zero_point);

absl::Status TfLiteTensorsToFloatsCalculatorU8::Process(CalculatorContext* cc) {
  RET_CHECK(!cc->Inputs().Tag(kTensorsTag).IsEmpty());

  const auto& input_tensors =
      cc->Inputs().Tag(kTensorsTag).Get<std::vector<TfLiteTensor>>();
  // TODO: Add option to specify which tensor to take from.
  const TfLiteTensor* raw_tensor = &input_tensors[0];
  const uint8_t* raw_bytes = raw_tensor->data.uint8;
  int num_values = 1;
  for (int i = 0; i < raw_tensor->dims->size; ++i) {
    RET_CHECK_GT(raw_tensor->dims->data[i], 0);
    num_values *= raw_tensor->dims->data[i];
  }

  float scale, zero_point;
  GetQuantizationParams(raw_tensor, scale, zero_point);

  if (cc->Outputs().HasTag(kFloatTag)) {
    // TODO: Could add an index in the option to specifiy returning one
    // value of a float array.
    RET_CHECK_EQ(num_values, 1);
    cc->Outputs().Tag(kFloatTag).AddPacket(
        MakePacket<float>(DQ(raw_bytes[0])).At(cc->InputTimestamp()));
  }
  if (cc->Outputs().HasTag(kFloatsTag)) {
    auto output_floats = absl::make_unique<std::vector<float>>(num_values);
        // raw_bytes, raw_bytes + num_values);

    std::transform(raw_bytes, raw_bytes + num_values,
                   std::begin(*output_floats),
                   [scale, zero_point](const uint8_t q) { return DQ(q); });

    cc->Outputs()
        .Tag(kFloatsTag)
        .Add(output_floats.release(), cc->InputTimestamp());
  }

  return absl::OkStatus();
}
}  // namespace mediapipe
