
Leap Motion Peripheral/Rigel Viewer
===================================

Richard Cownie, Leap Motion, 2019-05-01

Introduction
============

The viewer program displays video from a Leap Motion Peripheral
(with the leapuvc firmware to enable access as a normal UVC device)
or Rigel hand-tracking device.

Command-line options
====================

By default, the viewer will use the lowest-numbered Peripheral or Rigel
device - if both a Peripheral and Rigel are present, the option
"--product peripheral" or "--product rigel" may be used to select the device.

By default, the viewer will choose the device's lowest frames-per-second
(and highest resolution).  The choice can be constrained by specifying
any or all of "--fps <num>", "--width <pixels>", and/or "--height <pixels>",
and the supported formats will be searched in ascending order of frame-rate
until an exact match is found.

When viewing low-resolution video, "--magnify <num>" will produce a 
larger image, replicating each pixel without smoothing/interpolation.

Other control settings may be given on the command-line, and may be changed
using sliders in the "Controls" window:

    --analog_gain <num>   analog gain
    --exposure <num>      exposure in microseconds
    --leds <on|off>       IR led illumination on or off

  Peripheral also supports these controls:
    --digital_gain <num>  digital gain (brightness)
    
    --gamma <on|off>      gamma-correction on or off
    --hdr <on|off>        high dynamic range on or off

Keyboard controls
=================

Keyboard input to the video window can be used thus:

    's'  Stop streaming display (useful to freeze a single frame)
    'g'  Go restart streaming display
    'q'  Quit the viewer

Building the viewer from source
===============================

Source code for the viewer is at https://github.com/leapmotion/librealuvc

The viewer depends on OpenCV-4.0.1.  Prebuilt OpenCV binaries for 64bit Windows can
be downloaded from:
  https://sourceforge.net/projects/opencvlibrary/files/4.0.1/opencv-4.0.1-vc14_vc15.exe

The viewer is built using CMake and Visual Studio. The CMAKE_PREFIX_PATH should be set 
appropriately to discover the OpenCVConfig.cmake