#!/bin/bash

# Run this script from the mediapipe root directory
bazel build \
    --config=dad_config \
    --cpu=aarch64 \
    --compiler=coral-gcc \
    --copt=-DLIBYUV_DISABLE_NEON \
    --compilation_mode=opt \
    --define MEDIAPIPE_DISABLE_GPU=1 \
    --define MEDIAPIPE_EDGE_TPU=pci \
    --linkopt=-l:libusb-1.0.so \
    --define darwinn_portable=1 \
    mediapipe/examples/coral:hand_tracking_tpu_client
    # --copt=-g \
    # --copt=-O0 \

# Make the binary writable for all users to allow debugging, scp, rewrite, etc.
chmod a+w bazel-bin/mediapipe/examples/coral/hand_tracking_tpu_client
chmod a+w bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so

# rsync -avz \
#     bazel-out/aarch64-dbg/bin/mediapipe/examples/coral/hand_tracking_tpu_client \
#     bazel-out/aarch64-dbg/bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
#     mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
#     mediapipe/modules/hand_landmark/handedness.txt \
#     X:/home/mendel/mediapipe
scp \
    bazel-bin/mediapipe/examples/coral/hand_tracking_tpu_client \
    bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
    mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
    mediapipe/modules/hand_landmark/handedness.txt \
    X:/home/mendel/mediapipe


rsync -az \
    --info=progress2 \
    --exclude='cmake/' \
    $HOME/coral/buildsys/install/opencv/lib/*.so* \
    X:/home/mendel/mediapipe

# When you use rsync with a wildcard like .so, the shell expands
# the pattern to a list of files, and rsync copies the symlink 
# itself (not the file it points to) unless you tell it otherwise. 
# If the symlink target does not exist on the destination, you 
# get a broken symlink.

# With scp, the shell expands the symlinks to their targets, so 
# the actual files are copied, not the symlinks.

# How to fix with rsync:
# Add the -L option to rsync to copy the files symlinks point to, not
# the symlinks themselves.

# The -L (or --copy-links) option tells rsync to follow symlinks and 
# copy the actual files.

# This will avoid broken symlinks on the destination.

rsync -azL \
    --info=progress2 \
    $HOME/coral2/x-tool-build/aarch64-linux-gnu/aarch64-linux-gnu/lib64/*.so* \
    X:/home/mendel/mediapipe
