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
	mediapipe/examples/coral:hand_tracking_$VER
	# --define tflite_with_xnnpack=false \
	# --cxxopt='-std=c++17' \

exit 0

# Copy to target
APP=$HOME/fork/mediapipe/bazel-bin/mediapipe/examples/coral/hand_tracking_$VER

chmod +rwx $APP
rsync -az --info=progress2 \
	$APP \
	$TARGET:/home/mendel

# exit 0

# Run executable on target
ssh -n -f $TARGET \
  "sh -c 'cd /home/mendel; \
	export DISPLAY=:0.0; \
	GLOG_logtostderr=1; \
  nohup gdbserver localhost:3000 \
  hand_tracking_tpu --calculator_graph_config_file hand_tracking_tpu.pbtxt > output.log 2>&1 &'"
