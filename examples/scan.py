import numpy as np
import cv2
import pyrealuvc

for id in [0,1,2,3]:
 print("VideoCapture(%d) ..." % id)
 cap = pyrealuvc.VideoCapture(id)
 if (cap.isOpened()):
   print("camera id %d: vendor_id 0x%04x product_id 0x%04x"
     % (id, cap.get_vendor_id(), cap.get_product_id()))
