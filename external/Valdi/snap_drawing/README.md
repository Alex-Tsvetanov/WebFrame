# SnapDrawing

A fast, native and cross-platform drawing and UI system leveraging the Skia library.

## When should you use SnapDrawing?

In most cases, you should use Valdi for building a cross-platform UI. Valdi is built on top of SnapDrawing for Android.
There are some cases however where using SnapDrawing directly might make sense:
- You need to render some form of UI into a bitmap or a texture that you control
- You want to control the draw loop yourself, or even draw a one-off UI without a loop

## Demo app

The src/demo folder contains a demo CLI desktop app that leverages SnapDrawing to setup a scene using layers with a given image or video, and generates an output as a PNG or MP4 file. The image demo renders the scene once and exports the result. The video demo creates the scene and updates it on every decoded video buffer with its timestamp. It includes a fade-out animation which shows how a SnapDrawing layer scene can be updated through a video timeline.

To process an image:
```sh
bzl run //src/snap_drawing:snap_drawing-demo -- image -i <path_to_input_image> -o <path_to_output_image>
```
To process a video:
```sh
bzl run //src/snap_drawing:snap_drawing-demo -- video -i <path_to_input_video> -o <path_to_output_video>
```
