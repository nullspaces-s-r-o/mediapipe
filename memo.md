
### Palm detection on folder with images

compile: 
`bash ./mediapipe/examples/coral/cross_compile_palm_detection_tpu.sh`

````
./palm_detection_tpu_video \
	--calculator_graph_config_file palm_detection_live.pbtxt \
	--input_video_path ./hand_images/hands%02d.jpg \
	--output_video_path hands_out.avi
```

