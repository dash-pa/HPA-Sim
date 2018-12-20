
 -- USING arduinomicro_joystick.ino ARDUINO FIRMWARE --
To install joystick library required to compile joystick firmware for Arduino Micro:
Copy the folder
ArduinoJoystickLibrary-version-1.0.zip/ArduinoJoystickLibrary-version-1.0/Joystick
To your Arduino libraries folder. Ex:
C:/programs/arduino-1.8.5/libraries/Joystick
Restart the Arduino IDE and you should be able to compile arduinomicro_joystick.ino
Make sure to select the "Arduino/Genuino Micro" board before compiling.
The Micro's port may claim to be a Leonardo the first time you upload to it. Ignore it, that's normal.
 ------------------------------------------------------

 -- Installing RPi software on a SD card --
I recommend that you check the integrity of the RPi image file after you download it. You can do so by downloading the checksum file (image file).md5sum, and then running the following command in the same directory as the RPi image file:
md5sum -c sumpac_rpi_shrunk.img.gz.md5sum
If it checks out, the output will look like "sumpac_rpi_shrunk.img.gz: OK".
First find what device your SD card is. To do this, you can use Gparted. (sudo apt-get install -y gparted && sudo gparted)
To flash your SD card, use the following command with $SD_CARD replaced by your sd card device file. (ex: /dev/sde)
MAKE SURE YOU USE THE RIGHT DEVICE FILE, IF YOU USE THE WRONG ONE YOU COULD WIPE YOUR COMPUTER.
gzip -dc sumpac_rpi_shrunk.img.gz | sudo dd bs=4M of=$SD_CARD
 ------------------------------------------

 -- Hardware Connections --
The RPi's GPIO14/UART0_TXD/pin8 (see rpi_pinout.jpg, pin8 means the purple "8" pin in that image) should be connected to the Arduino Micro's "RX1" pin (printed on the Micro's board right next to the pin)
OPTIONAL: The RPi's GPIO15/UART0_RXD/pin10 should be connected to the Micro's "TX0". This is currently not used.
Any of the RPi's "Ground" pins (recommend pin9 or pin6 for proximity to RPi's RX/TX pins) should be connected to any of the Micro's "GND" pins. Only one connection is necessary, but if you have spare wires in a bundle cable then you can keep them tidy by using them to connect extra Ground pins to GND pins.
The RPi's "5V Micro USB Power" needs to be connected to a USB power source. For maximum reliability, use a 5V 2-Amp phone charger. I've powered it off of a computer USB port sucessfully, but it might not get enough power that way.
The Arduino Micro can just be plugged into a computer USB port, no special power requirements.
 --------------------------


 -- using RPi --
/boot is the 69MiB (70.7MB) fat32 filesystem on the 6th partition of the SD card once everything has been installed correctly. When the SD card is auto-mounted in Windows or another OS, that filesystem should be one of the volumes that shows up.
The /boot/sumpac/config.xml file is the config file for the BLE converting program. the ble_dev_addr will need to be set to the BLE device address of the Vector 3 pedals for everything to work.
Logging can be turned on or off by creating or deleting (respectively) /boot/sumpac/debug.txt, the contents of the file are irrelevent. Log files will show up in /boot/sumpac when debugging is enabled.
Logging should be considered a debugging feature and be turned off during normal use because I haven't written anything to prevent the logs from becoming enormous over time.
 ---------------


 -- rpi_files --
 - This section can be ignored unless you're trying to install everything on the RPi from scratch/fresh Debian install -
This folder contains the files that were modified/added to turn a fresh install of Debian into a working RPi BLE->UART converter.
home_pi was /home/pi, there were other files from the Debian install that were irrelevent and thus removed from this folder.
Of particular interest is home_pi/install_log.txt, which contains my notes of what I was doing to get the libraries installed and startup scripts enabled.
home_pi/startup.bash is actually supposed to be a link to /etc/init.d/startup.bash, so it should be put there and linked if installing all this from scratch.
home_pi/ OscarAcena-pygattlib-blahblahblah and pybluez can be downloaded from the internet. Also, you only need one, I just downloaded both while figuring out how to get pygattlib installed. See install_log.txt for a little more info.
boot_sumpac was /boot/sumpac, it contains the config.xml file for the ~/ble.py program. ~/ble.py and ~/startup.bash use this directory. Config files were put in /boot because it's a fat32 filesystem that Windows can easily mount.
 ---------------
