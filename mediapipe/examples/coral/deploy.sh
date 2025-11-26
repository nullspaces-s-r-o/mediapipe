#!/bin/bash 
cp -v \
    $HOME/mediapipe/mediapipe/examples/coral/hand_tracking_tpu_lib.h \
    $HOME/mediapipe/mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
    $HOME/mediapipe/mediapipe/modules/hand_landmark/handedness.txt \
    $HOME/mediapipe/bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
    ~/hand_app