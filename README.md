# QIDI_connect
Webbrowser based Communication with QIDI 3D printers for Linux

Version 0.5 (May,24  2020)

![qidi_connect web interface](https://github.com/dj0abr/QIDI_connect/blob/master/sample_small.png)

All in one nice GUI in a Web Browser:
* show QIDI 3D printer status
* show files on SD card
* gcode file upload
* start 3d print job
* embed webcams

# System requirements:
a LINUX computer, like an SBC (Raspberry PI, Odroid...) but also any other Linux Destop PC.
A network connection to your QIDI 3D printer

# Current functions

- automatically find the IP address of the QIDI 3D printer. One printer is currently supported, until QIDI sends me a second one for free:-)
- reading the machine parameters
- reading the printer staus
- diplaying these information on the screen
- lists files on the SD card
- file upload to the 3D printer.
- start print job
- view status in a bowser

# build the software
```json
make clean
make
```
# prepare the system
```json
run the script:

./prepare_ubuntu
```
# prepare php for uploading big files
```js
as root open the php.ini file. It is located in /etc/php/..../php.ini  (.... depends on the version number)

change these lines:

upload_max_filesize
post_max_site
memory_limit

set to a value larger then your largest gcode file, i.e.: 300M

After changing the file restart apache or reboot (sudo service spache2 restart)
```

# start the software
```js
in the folder where qidi_connect is located: 
sudo ./qidi_connect [options]

(sudo is needed because qidi_connect needs to write files into the apache html folder)

options:
-v ... verbose, show a lot of debugging messages
-V ... version information
```
# stopping this software
```json
you can safely stop the software either by entering x-Enter or by pressing Ctrl-C.
```
# using the software
```json
When you start the software watch the messages in the terminal. 
It shows if your printer was found and if status messages can be downloaded.
If all works as expected, then open a WebBrowser in any device in your 
local home network and browse to the IP address of you Raspberry PI or other 
Linux computer used for this software.

The GUI should be easy to use without any other instructions
```
# embedding a camera
```json
this has nothing to do with this software, but makes the GUI looking nicer.

I am doing this on a raspberry PI. I have a standard raspberry PI 
camera connected to the camera port.

The following is already done by the script: prepare_ubuntu:

first install mpeg_streamer from the github repo: mjpg-streamer-experimental

Then start your camera with this command:
./mjpg_streamer -i "./input_uvc.so -vf true -hf true  -f 25 -r 640x480" -o "./output_http.so -p 8086 -w ./www"

It will stream the video to port 8086.
Now open the file index.html, look for 8086 and modify the URL so 
it points to your computer.

The website index.html is prepared to display two video streams. 
If you don't need it then simply delete the video links (see comment in index.html)
```
# access from outside your home through the internet
```json
if you plan to use this software from a publich place, 
then set a password in handler.php !!!!!!!
Do not use the default password 1234 if the ports 
are open for the public !!!!!!
```
# credits
this software uses a modified version of gcodestat written by 
Bogdan Kecman "arhi"
from the github repository:
https://github.com/arhi/gcodestat
