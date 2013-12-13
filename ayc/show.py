# coding=utf-8
__author__ = 'stasstels'


import cv2
import sys

image = sys.argv[1]
targets = sys.argv[2]

# Load an color image in grayscale
img = cv2.imread(image, cv2.IMREAD_COLOR)

with open(targets, "r") as f:
    for line in f:
	print line
        (_, x, y) = line.split()
        cv2.circle(img, (int(x), int(y)), 20, (255, 0, 255), -1)

cv2.namedWindow("image", cv2.WINDOW_NORMAL)
cv2.imshow('image', img)
cv2.waitKey(0)
cv2.destroyAllWindows()

