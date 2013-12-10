# coding=utf-8
__author__ = 'stasstels'


import cv2

# Load an color image in grayscale
img = cv2.imread('test/1.bmp', cv2.IMREAD_COLOR)

with open("out.txt", "r") as f:
    for line in f:
        (_, x, y) = line.split()
        cv2.circle(img, (int(x),int(y)), 2, (0, 255, 255), -1)

cv2.imshow('image', img)
cv2.waitKey(0)
cv2.destroyAllWindows()

