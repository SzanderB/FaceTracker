import cv2


# face_detect
# Detects faces, returns bounding box dimensions
# Param 'frame': video or image frame
# Param 'faceCascade': path to cascade xml file
# Returns object array containing (x,y,w,h) of bounding boxes.
def face_detect(frame, faceCascade):
    """
        Inputs: frame (numpy array) and faceCascade XML model
        Output: faces (https://docs.opencv.org/3.4/db/d28/tutorial_cascade_classifier.html)
    """
    if len(frame.shape)==3:
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    else:
        gray = frame

    # Detect faces in an frame
    faces = faceCascade.detectMultiScale(
        gray,
        scaleFactor=1.1,
        minNeighbors=2,
        minSize=(10, 10)
    )
    return faces

# draw_face_boxes
# Draws bounding boxes on image
# Param 'frame': video or image frame
# Param 'faces': faces object array containing (x,y,w,h) of bounding boxes.
# Returns image with bounding box overlay
def draw_face_boxes(frame, faces):
    # draw bounding boxes for all faces in frame
    if faces is None:
        return frame
    for (x, y, w, h) in faces:
        cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
    return frame
