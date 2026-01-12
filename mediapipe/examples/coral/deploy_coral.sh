#!/bin/bash

# Where X is configured in ~/.ssh/config as the Coral device
# Host X
#     HostName 192.168.0.249
#     User mendel

set -e

compilation_mode="opt"
# copt="--copt=-g --copt=-O0"
copt="--copt=-O2"

# Run this script from the mediapipe root directory
bazel build \
    --config=dad_config \
    --cpu=aarch64 \
    --compiler=coral-gcc \
    --copt=-DLIBYUV_DISABLE_NEON \
    --compilation_mode=$compilation_mode \
    $copt \
    --define MEDIAPIPE_DISABLE_GPU=1 \
    --define MEDIAPIPE_EDGE_TPU=pci \
    --linkopt=-l:libusb-1.0.so \
    --define darwinn_portable=1 \
    mediapipe/examples/coral:libhand_tracking_tpu_lib.so

bazel build \
    --config=dad_config \
    --cpu=aarch64 \
    --compiler=coral-gcc \
    --copt=-DLIBYUV_DISABLE_NEON \
    --compilation_mode=$compilation_mode \
    $copt \
    --define MEDIAPIPE_DISABLE_GPU=1 \
    --define MEDIAPIPE_EDGE_TPU=pci \
    --linkopt=-l:libusb-1.0.so \
    --define darwinn_portable=1 \
    mediapipe/examples/coral:hand_tracking_tpu_client    

bazel build \
    --config=dad_config \
    --cpu=aarch64 \
    --compiler=coral-gcc \
    --copt=-DLIBYUV_DISABLE_NEON \
    --compilation_mode=$compilation_mode \
    $copt \
    --define MEDIAPIPE_DISABLE_GPU=1 \
    --define MEDIAPIPE_EDGE_TPU=pci \
    --linkopt=-l:libusb-1.0.so \
    --define darwinn_portable=1 \
    @libedgetpu//tflite/public:libedgetpu_direct_pci.so


INSTALL_DIR=$HOME/$BOARD/buildsys/install/libedgetpu
mkdir -p $INSTALL_DIR/direct/aarch64 #throttled/aarch64

# Change permissions to allow overwriting existing files
chmod a+w $INSTALL_DIR/direct/aarch64/libedgetpu.so.1.0

# Copy the built .so file (adjust the path as needed)
cp \
    bazel-bin/external/libedgetpu/tflite/public/libedgetpu_direct_pci.so \
    $INSTALL_DIR/direct/aarch64/libedgetpu.so.1.0


# Create symlinks
ln -sf libedgetpu.so.1.0 $INSTALL_DIR/direct/aarch64/libedgetpu.so.1
ln -sf libedgetpu.so.1 $INSTALL_DIR/direct/aarch64/libedgetpu.so

cp \
    bazel-mediapipe/external/libedgetpu/tflite/public/edgetpu.h \
    $INSTALL_DIR

#############################
# Build glog shared library
#############################
# bazel build \
#     --config=dad_config \
#     --cpu=aarch64 \
#     --compiler=coral-gcc \
#     --compilation_mode=$compilation_mode \
#     $copt \
#     @com_github_glog_glog//:glog

# GLOG_INSTALL_DIR=$INSTALL_DIR/../glog
# mkdir -p $GLOG_INSTALL_DIR/include
# mkdir -p $GLOG_INSTALL_DIR/lib

# chmod a+w bazel-bin/external/com_github_glog_glog/libglog.a

# # Copy the built glog .so file
# cp \
#     bazel-bin/external/com_github_glog_glog/libglog.a \
#     $GLOG_INSTALL_DIR/lib/

# cp -r \
#     bazel-bin/external/com_github_glog_glog/src/glog/* \
#     $GLOG_INSTALL_DIR/include/

#############################
# Build tensorflow
#############################
bazel build \
    --config=dad_config \
    --cpu=aarch64 \
    --compiler=coral-gcc \
    --copt=-DLIBYUV_DISABLE_NEON \
    --compilation_mode=$compilation_mode \
    $copt \
    --define MEDIAPIPE_DISABLE_GPU=1 \
    --define MEDIAPIPE_EDGE_TPU=pci \
    --linkopt=-l:libusb-1.0.so \
    --define darwinn_portable=1 \
    @org_tensorflow//tensorflow/lite:libtensorflowlite.so
TF_INSTALL_DIR=$INSTALL_DIR/../tensorflow
mkdir -p $TF_INSTALL_DIR/include
mkdir -p $TF_INSTALL_DIR/lib

rsync -av --include='*/' --include='*.h' --exclude='*' \
  bazel-mediapipe/external/org_tensorflow/tensorflow/lite/ \
  "$TF_INSTALL_DIR/include"

# Copy tenforflow lite headers to stage directory (include)
mkdir -p $STAGE/include/tensorflow
rsync -av --include='*/' --include='*.h' --exclude='*' \
  bazel-mediapipe/external/org_tensorflow/tensorflow/lite \
  "$STAGE/include/tensorflow"

# Copy tensorflow compiler headers to stage directory (include)
rsync -av --include='*/' --include='*.h' --exclude='*' \
  bazel-mediapipe/external/org_tensorflow/tensorflow/compiler \
  "$STAGE/include/tensorflow/"

# Copy tensorflow lite shared library to stage directory (lib)
chmod a+w bazel-bin/external/org_tensorflow/tensorflow/lite/libtensorflowlite.so
mkdir -p $STAGE/lib
cp \
    bazel-bin/external/org_tensorflow/tensorflow/lite/libtensorflowlite.so \
    $STAGE/lib/

#############################
# Tensorflow schema_conversion_utils
#############################
bazel build \
    --config=dad_config \
    --cpu=aarch64 \
    --compiler=coral-gcc \
    --copt=-DLIBYUV_DISABLE_NEON \
    --compilation_mode=$compilation_mode \
    $copt \
    --define MEDIAPIPE_DISABLE_GPU=1 \
    --define MEDIAPIPE_EDGE_TPU=pci \
    --linkopt=-l:libusb-1.0.so \
    --define darwinn_portable=1 \
    @org_tensorflow//tensorflow/compiler/mlir/lite/schema:schema_conversion_utils
# Copy tensorflow schema_conversion_utils headers to stage directory (include)
chmod a+w bazel-bin/external/org_tensorflow/tensorflow/compiler/mlir/lite/schema/libschema_conversion_utils.a
mkdir -p $STAGE/lib
cp \
    bazel-bin/external/org_tensorflow/tensorflow/compiler/mlir/lite/schema/libschema_conversion_utils.a \
    $STAGE/lib/

#############################
# Flatbuffers
#############################
# Flatbuffers were compiled as part of TensorFlow build above
# Copy tenforflow lite headers to stage directory (include)
mkdir -p $STAGE/include/flatbuffers
rsync -av --include='*/' --include='*.h' --exclude='*' \
  bazel-mediapipe/external/flatbuffers/include/flatbuffers/ \
  "$STAGE/include/flatbuffers"

# Copy tensorflow lite shared library to stage directory (lib)
chmod a+w bazel-bin/external/flatbuffers/src/libflatbuffers.a
mkdir -p $STAGE/lib
cp \
    bazel-bin/external/flatbuffers/src/libflatbuffers.a \
    $STAGE/lib/



# Repeat for throttled if needed, or for other variants

# Make the binary writable for all users to allow debugging, scp, rewrite, etc.
chmod a+w bazel-bin/mediapipe/examples/coral/hand_tracking_tpu_client
chmod a+w bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so

# rsync -avz \
#     bazel-out/aarch64-dbg/bin/mediapipe/examples/coral/hand_tracking_tpu_client \
#     bazel-out/aarch64-dbg/bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
#     mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
#     mediapipe/modules/hand_landmark/handedness.txt \
#     X:/home/mendel/mediapipe

# Using scp instead of rsync to avoid broken symlinks for OpenCV libs
# Compy hand tracking client files to the device
# scp \
#     bazel-bin/mediapipe/examples/coral/hand_tracking_tpu_client \
#     bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
#     mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
#     mediapipe/modules/hand_landmark/handedness.txt \
#     X:/home/mendel/mediapipe


# Copy OpenCV libraries to the device
# rsync -az \
#     --info=progress2 \
#     --exclude='cmake/' \
#     $HOME/coral/buildsys/install/opencv/lib/*.so* \
#     X:/home/mendel/mediapipe

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

# Copy Coral toolchain libraries to the device
# rsync -azL \
#     --info=progress2 \
#     $HOME/coral2/x-tool-build/aarch64-linux-gnu/aarch64-linux-gnu/lib64/*.so* \
#     X:/home/mendel/mediapipe

# Installation package as zip file
# zip -j \
#     mediapipe_coral_hand_tracking_tpu_client_package.zip \
#     bazel-bin/mediapipe/examples/coral/hand_tracking_tpu_client \
#     bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
#     mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
#     mediapipe/modules/hand_landmark/handedness.txt \
#     $HOME/coral/buildsys/install/opencv/lib/*.so* \
#     $HOME/coral2/x-tool-build/aarch64-linux-gnu/aarch64-linux-gnu/lib64/*.so* \
#     mediapipe/examples/coral/landmark_model_full_integer_quant_edgetpu.tflite \
#     mediapipe/examples/coral/palm_model_full_integer_quant_edgetpu.tflite

LIB_HAND_TRACKING_TPU_DIR=$INSTALL_DIR/../libhand_tracking_tpu
mkdir -p $LIB_HAND_TRACKING_TPU_DIR

# cp \
#     mediapipe_coral_hand_tracking_tpu_client_package.zip \
#     $LIB_HAND_TRACKING_TPU_DIR/


# Create standard installation directory for hand tracking TPU library
cp \
    bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so \
    mediapipe/modules/hand_landmark/handedness.txt \
    mediapipe/examples/coral/hand_tracking_tpu_lib.h \
    mediapipe/examples/coral/graphs/hand_tracking_tpu.pbtxt \
    mediapipe/examples/coral/landmark_model_full_integer_quant_edgetpu.tflite \
    mediapipe/examples/coral/palm_model_full_integer_quant_edgetpu.tflite \
    $LIB_HAND_TRACKING_TPU_DIR/

# Distribute the installation package to the Coral device stage directory
mkdir -p $STAGE/include/hand_tracking_tpu_lib
cp mediapipe/examples/coral/hand_tracking_tpu_lib.h $STAGE/include/hand_tracking_tpu_lib/
cp bazel-bin/mediapipe/examples/coral/libhand_tracking_tpu_lib.so $STAGE/lib/



# Copy the installation package to the Coral device
# scp \
#     mediapipe_coral_hand_tracking_tpu_client_package.zip \
#     X:/home/mendel/mediapipe/libhand_tracking_tpu/


