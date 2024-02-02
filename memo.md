
### Palm detection on folder with images

compile: 
`bash ./mediapipe/examples/coral/cross_compile_palm_detection_tpu.sh`

```
./palm_detection_tpu_video \
	--calculator_graph_config_file palm_detection_live.pbtxt \
	--input_video_path ./hand_images/hands%02d.jpg \
	--output_video_path hands_out.avi
```

### hand tracking tpu

compile:
`bash mediapipe/examples/coral/cross_compile_hand_tracking_tflite_tmp.sh`

run:
`./hand_tracking_tpu --calculator_graph_config_file hand_tracking_tpu.pbtxt`
