Program code for face tracking software. There is a video demonstration showing the final product, https://youtu.be/4eWTQd3u4Io.

Thanks!


# Description
Project Idea: Security Camera that tracks your face.

Materials: USB webcam, Beaglebone Black, TIVA C launchpad, pan/tilt setup wtih servos, and uP Kit with joystick, buttons, servo module, LCD, etc

To start, I first tried to get communication between my host computer and the beaglebone. This was achieved through the internet and what's called a socket. To get it to function, I had to first understand and find the IP address of the Beaglebone and the host computer. This allows them to find each other. But to connect, they need a specific port that they need to agree upon to initialize the connection. This was programmed in python using the Socket library. 

Once I could transmit that data, I attempted to work on get the camera working on the Beaglebone. After a few frustrating hours of tinkering and youtubing, I went back to my host computer and tried to set up the facial detection there. Using cv2, I could open the camera, send the frame through a facial detection algorithm (haarcascades, which is a moving window filter that has general applications for facial detection) and it would provide the bounding box. With the coordinates, I drew the bounding box over the image and displayed it on my PC.

After getting that to work, I worked to set that up on my Beaglebone. I forgot to mention, but accessing the beaglebone requires SSHing into it and running only command line things. I enjoyed it a lot. One of my issues was that python was not downloading CV2 properly. Connecting to the internet was its own challenge, as I needed a network that was non-authenticating and had ethernet access (I did not have either readily available). After uninstalling and reinstalling, specifying the exact packages I needed, I could get python to include cv2, open a camera and take frames. After programming the facial detection part like already done on the host PC, I got it to properly output the coordinates of the bounding box to the host PC. However, the algorithm was pretty slow, around 1 frame a second. Since I was trying to do a realtime project and I would say that I had a firm deadline, that would not cut it. I researched into the algorithm, tried different filters and messed with the parameters. The best I could get it was around 2 frames a second. It also had poor accuracy.

I believed that using a neural net filter may help with the accuracy and maybe the speed. To do so, i needed to manually download the zip files for cv2 and unpackage and install them myself. After, I found that I was wrong and it slowed execution time significantly. Lastly, I tried a LBF filter for its speed. This worked wonderfully and got to around 5-10 frames a second with decent accuracy.

After this, I needed communication between the Beaglebone and the TIVA C launchpad. This was achieved through UART. Setting it up in python was simple as I just needed to use the Serial library. On the TIVA side, I made an interrupt that would receive the data and push it to a stack to be processed by the threads later. I sent over the boundaries of the box to display on the LCD as well as to use in the control of the camera's pan/tilt mount.

The threads on the TIVA were as follows:
Background: Idle thread (to avoid all threads sleeping), Face_Detect thread (draws bounding box on LCD when detected), Read_Buttons thread (responds to interrupt to process button press to switch between automatic and manual control of motors), Motor Control thread (Brunt of the work. Based on global var to be in manual/automatic mode. In manual mode, it reads a FIFO of joystick values and moves the motors based on its position. For automatic mode, I created a simple K controller that takes in the bounding box and outputs the corrective movements needed for centering the camera on the face.

Periodic: Get Joystick (read joystick values every 50 ms)

Aperiodic: UART (received from Beaglebone), and Button (for switching manual/auto)

It took a lot of tinkering of values, but finally got a working K controller that followed your face. It was not very fast, I think there were several problems. The camera was very slow, it felt like there was almost around a second delay from when it took the picture to when it was available to process. Also, it took time to get the facial detection running, so overall a large delay. If moving slowly, it worked and would center on my face. However, since I could not make the controller move the camera too fast, or it would move me out of frame. The ending product worked, it was just a little slow.

I realize months later that the speed also had to do with the fact that my face took up 70% of the frame already. If I backed up, I would be less of the screen and therefore the adjustments for centering me would be easier for the process to calculate and realize a change within that 4fps I was able to obtain.
