import numpy as np
import face_recognition
import os

r, w = map(int, sys.argv[1:3])

save_root = '/home/eatmystyle/Pictures/features' # direction to features file
features = np.loadtxt(save_root, delimiter=',') # read our features

img = face_recognition.load_image_file('/home/eatmystyle/Pictures/717958.jpg')

enc= face_recognition.face_encodings(img) #encode faces
if np.array(enc).shape[0] < 1: # no faces ddetected
    print("I wasn't able to locate any faces in the image. Check the image files. Aborting...")
    os.write(w, b'0')
    quit()
elif np.array(enc).shape[0] > 1: # more than 1 face detected
    print("Too many faces detected. Check the image files. Aborting...")
    os.write(w, b'0')
    quit()

answer = face_recognition.compare_faces(enc, features) # predictions

if np.sum(np.array(answer)) == 1:
    os.write(w, b'1')
else:
    os.write(w, b'0')
quit()

