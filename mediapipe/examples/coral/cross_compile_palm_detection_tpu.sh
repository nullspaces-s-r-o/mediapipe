set -ex

VER=tpu

# Build executable
bazel build \
	--config=coral_config \
	--define darwinn_portable=1 \
	--define MEDIAPIPE_DISABLE_GPU=1 \
	--define MEDIAPIPE_EDGE_TPU=pci \
	--compilation_mode dbg \
	--copt=-g \
	--cpu=aarch64 \
	--verbose_failures \
	--jobs 10 \
	mediapipe/examples/coral:palm_detection_$VER
	# --sandbox_debug \
	# --define tflite_with_xnnpack=false \
	# --cxxopt='-std=c++17' \

