# QIDI_connect
Communication with QIDI 3D printers for Linux

Download is MUCH faster and more reliable compared to the original QIDI software

# System requirements:
a LINUX computer, like an SBC (Raspberry PI, Odroid...) but also any other Linux Destop PC.
A network connection to your QIDI 3D printer

# Current functions

this is work in progress and at a very early stage. Please be patient, more will come soon

Currently implemented functions:

- automatically find the IP address of the QIDI 3D printer. One printer is currently supported, until QIDI sends me a second one for free:-)
- reading the machine parameters
- reading the printer stagit push origin mastertus
- diplaying these information on the screen
- lists files on the SD card
- file upload to the 3D printer. Filename given as command line argument -u, see below

# future plans

- nice GUI using any Web Browser on any device

# build the software

make clean

make

# start the software

./qidi_connect [options]

options:
-v ... verbose, show a lot of debugging messages
-V ... version information
-u filename ... filename for a gcode file upload using the 'u' function (see runtime commands)

# runtime commands

when this software is running, you can enter these command keys (followed by pressing ENTER):

- 'f' and Enter: a list of files from the SD card is shown
- 'u' and Enter: uploads a file. Filename given as command line argument "-u filename"
- 'x' and Enter: exits this program
