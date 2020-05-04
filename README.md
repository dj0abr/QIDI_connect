# QIDI_connect
Communication with QIDI 3D printers

# System requirements:
a LINUX computer, like an SBC (Raspberry PI, Odroid...) but also any other Linux Destop PC.
a network connectipn to your QIDI 3D printer

# Current functions

this is work in progress and at a very early stage. Please be patient, more will come soon

Currently implemented functions:

- automatically find the IP address of the QIDI 3D printer. One printer is currently supported, until QIDI sends me a second one for free:-)
- reading the machine parameters
- reading the printer status
- diplaying these information on the screen
- by pressing the 'f' key a list of files from the SAD card is shown
- pressing 'x' exits this program
- pressing 'u' uploads a dummy file: testfile.gcode to the printer's SSD card

# work in progress

- full functional gcode file upload

# future plans

- nice GUI using any Web Browser on any device

# build the software

make clean
make

# run the software

./qidi_connect

then watch the output on the screen and try the keys f, u and x

