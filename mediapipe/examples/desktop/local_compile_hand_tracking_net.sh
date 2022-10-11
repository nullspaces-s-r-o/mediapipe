set -ex

# Build executable
bazel build \
	--define MEDIAPIPE_DISABLE_GPU=1 \
	--compilation_mode opt \
	mediapipe/examples/desktop/hand_tracking:hand_tracking_net
	# --copt=-g \
	# --cxxopt='-std=c++17' \

export GLOG_logtostderr=1