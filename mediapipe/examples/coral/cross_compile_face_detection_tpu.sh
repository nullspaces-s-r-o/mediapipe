set -ex

# Build executable
bazel build \
	--config=coral_config \
	--define darwinn_portable=1 \
	--define MEDIAPIPE_DISABLE_GPU=1 \
	--define MEDIAPIPE_EDGE_TPU=pci \
	--compilation_mode dbg \
	--copt=-g \
	--cxxopt='-std=c++17' \
	mediapipe/examples/coral:face_detection_tpu

# Copy to target
APP=$HOME/fork/mediapipe/bazel-mediapipe/bazel-out/aarch64-dbg/bin/mediapipe/examples/coral/face_detection_tpu

chmod +rwx $APP
rsync -az --info=progress2 \
	$APP \
	$TARGET:/home/mendel

# # Run executable on target
# ssh -X -n -f $TARGET \
#   "sh -c 'cd /home/mendel; \
#   nohup gdbserver localhost:3000 \
#   face_detection_tpu --calculator_graph_config_file face_detection_desktop_live.pbtxt > output.log 2>&1 &'"