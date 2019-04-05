echo off
set XLIBS=D:/xlibs
set LRU=D:/rcownie/librealuvc-build
set PYTHONPATH=%LRU%/Release;%XLIBS%/opencv-4.0/python/cv2/python-3.6
set PATH=%LRU%/Release;%XLIBS%/opencv-4.0/x64/vc15/bin;%PATH%
echo on

python minimumExample.py
