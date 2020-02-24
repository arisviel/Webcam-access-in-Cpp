import numpy as np
import face_recognition
import os
import sys


w = int(sys.argv[1])

# making list of objects in containing folder
root = '/home/eatmystyle/Pictures/knows_persons/' #way to folder with imgs
save_root = '/home/eatmystyle/Pictures/' #way to save results, CANNOT be as root way!
imgs = list(sorted(os.listdir(os.path.join(root))))


# extract features for our images
inter = 0 #crutch
for img in imgs:
    img_array = face_recognition.load_image_file(root+img) #load image as np.array
    
    encoded = face_recognition.face_encodings(img_array) #extract features
    if np.array(encoded).shape[0] < 1: # no faces ddetected
        print("I wasn't able to locate any faces in the image", img, 'Check the image files. Aborting...')
        os.write(w, b'0')
        quit()
    elif np.array(encoded).shape[0] > 1: # more than 1 face detected
        print("Too many faces detected", img,"Check the image files. Aborting...")
        os.write(w, b'0')
        quit()
    
    #stacking image on axis 0
    if inter == 0:
        batch = encoded
    else:
        batch = np.concatenate((batch, encoded), axis=0)
    inter += 1


# Save features
np.savetxt((save_root + 'features') ,batch, delimiter=',')
# output
os.write(w, b'1')
quit()
