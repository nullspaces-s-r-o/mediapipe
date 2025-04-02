set -ex

VER=tpu

# Build executable
bazel build \
	--config=coral_config \
	--define darwinn_portable=1 \
	--define MEDIAPIPE_DISABLE_GPU=1 \
	--define MEDIAPIPE_EDGE_TPU=pci \
	--cpu=aarch64 \
	--verbose_failures \
	--jobs 10 \
	mediapipe/examples/coral:hand_tracking_$VER
	# --compilation_mode dbg \
	# --copt=-g \
	# --sandbox_debug \
	# --define tflite_with_xnnpack=false \
	# --cxxopt='-std=c++17' \

# exit 0

# Copy to target
APP=$HOME/mediapipe/bazel-bin/mediapipe/examples/coral/hand_tracking_$VER

chmod +rwx $APP

# copy the binary to the target
rsync -az --info=progress2 \
	$APP \
	$TARGET:/home/mendel

# copy mediapipe execution graph to the target
rsync -az --info=progress2 \
	$HOME/mediapipe/mediapipe/graphs/hand_tracking/hand_tracking_tpu.pbtxt \
	$RHOME/mediapipe

# Run executable on target
ssh -n -f $TARGET \
  "sh -c 'cd /home/mendel; \
	export DISPLAY=:0.0; \
	GLOG_logtostderr=1; \
  nohup gdbserver localhost:3000 \
  hand_tracking_tpu --calculator_graph_config_file hand_tracking_tpu.pbtxt > output.log 2>&1 &'"
