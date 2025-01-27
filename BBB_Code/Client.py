'''
Client.py
By: Szander Brenner

Takes an image from USB camera, processes it and sends if face is detected to TIVA to display on LCD
'''
from random import randint
from time import sleep
import serial
import cv2

from struct import *

# ------------------------------------ FPS ----------------------------------- #

FPS = 1
DELAY = 1/FPS

# -------------------------------- DIMENSIONS -------------------------------- #
LCD_W = 240
LCD_H = 320

RATIO = 0.5

SOCKET_W = int(LCD_W * RATIO)
SOCKET_H = int(LCD_H * RATIO)

MAXNUMFACES = 1

# ----------------------------------- MAIN ----------------------------------- #
if __name__=="__main__":
    print("hey")
    # get Cascade
    face_cascade = cv2.CascadeClassifier('./haarcascade_frontalface_default.xml')

    # set up UART
    ser = serial.Serial("/dev/ttyO4", baudrate=115200, bytesize=8, parity="N", stopbits=1)
    
    # Start Camera
    cam = cv2.VideoCapture(0)

    if not cam.isOpened:
        print('--(!)Error opening video capture')
        exit(0)

    while True:
        try:
            # wait FPS amount of Frames
            count = 0
            while count != FPS:
                ret, frame = cam.read()
                count += 1
            if frame is None:
                print('--(!) No captured frame -- Break!')
                break
            
            # resize and greyscale image
            frame = cv2.resize(frame, (LCD_W,LCD_H))
            frameResized = cv2.resize(frame, (SOCKET_W, SOCKET_H))
            frame_gray = cv2.cvtColor(frameResized, cv2.COLOR_BGR2GRAY)
            frame_gray = cv2.equalizeHist(frame_gray)

            # get faces
            faces = face_cascade.detectMultiScale(frame_gray)
            if len(faces) > 0:
                print("Face!")

            # limit the number of faces to MAXNUMFACES
            if len(faces) > MAXNUMFACES:
                faces = faces[:MAXNUMFACES]
            
            # if faces are found
            if len(faces) != 0:
                ser.write(1)
            else:
                ser.write(0)

        except KeyboardInterrupt:
            break



    # After the loop release the cap object 
    cam.release() 
    # Destroy all the windows 
    cv2.destroyAllWindows() 
    print("Program terminated.")