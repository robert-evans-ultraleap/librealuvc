#!/bin/sh
mkdir build
cd build
X=/d/xlibs
cmake \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_INSTALL_PREFIX=/d/xlibs/librealuvc-0.1 \
  -DCMAKE_PREFIX_PATH="$X/boost_1_70_0;$X/eigen-3.3.7;$X/opencv-4.1.0;$X/protobuf-3.8.0" \
  ..
make -j3

 