# DHTxxToUSBwithAVR
How to connect DHT11 or DHT22 sensort to usb using AVR.
There is also java reader app.


To connect DHT11 or DHT22 to USB:

Flash one of hex files from firmware/bin directory depending of used crystal and change fuses to 0xDF both.
Solder circuit from 'doc' directory and run DHTxx/bin/pl/nietamten/DHTxx/dht.jar to get readings.

Circuit hints:
- diodes act as voltage legulator and can be replaced with real one (see vusb page for details, take any diodes with usual voltage drop and it should work) 
- led can be ommited

GUI explanation:
- list of connected devices is updated while program is running but with some delay
- selecting device in list reads configuration
- to apply changed configuration click "apply settings"
- right controls are for "zooming"
	- click "set slider limits" 
	- use scrollbars to select how many data should be seen and skipped
	- click "apply slider values"

TODO (aka known bugs):
- change interval type to 32 bit int to allow intervals longer than 65535ms
- make GUI more intuitive
- java app not optimalized - every probe redraws whole plot and every point is drawed even tere is no space
- firmware code is unnecessarily complicated becouse of my past worries about need of frequenty calling usbPool() what is needed only for device type (CDC) from which I gave up for HID 

Tested with success on Windows and Linux.

There is not finished but working app in C++ builded in VS Community 2015 but without whole project. 
It uses FLTK from http://www.fltk.org/index.php and hidapi from http://www.signal11.us/oss/hidapi/ .

Firmware is using "Input Capture Unit" of AVR instead of "Pin Change" interrupt or polling.
It was intendet to run as CDC (like COM port) device but VUSB implementation of CDC and Windows 10 seems not work well.

About DHT
- there are probably several DHT11 versions, certainly the housing is different
- DHT11 documentation pecifies range of humidity reading as 20%-90% but  get readings like 10% or 93%
- DHT11 reading is dependent on voltage, between 3.3V and 5.1V I noticed differences between 5% and 3C; with DHT22 I not noticed that
- DHT11 in sometimes in some time ranges regularly gave unjustified single higher readings
- DHT11 seems to "stick" to certain readings and for the reading to change by one percent (for example) it need 5% humidity difference in some cases

Building:

(1) make hardware
- (can be ommited - for updating purposes) copy "usbdrv" directory from https://github.com/obdev/v-usb to "firmware" direcotry of this repository; set crystal value in Makefile (look at F_CPU, default is for 12MHz) and type "make hex" in "firmware" directory
- connect attiny44 throu USBASP to upload firmware and change fuses by running (other than USBASP programmers need script changes) 
	(a) upload.sh if building from source
	(b) bin/upload.sh <filename> where filename is one of hex files in this directory
- solder circuit and check it is detected by operating system (schema is in "doc" directory)

(2) make software (included binaries may not work on some machines)
- import prject from this repository to eclipse (manually or with egit from https://www.eclipse.org/egit/ File->Import->Git->Project from GIT->select 'dht' directory)
- (can be ommited - for updating purposes) copy to lib directory:
	- jna-xxx.jar from https://github.com/java-native-access/jna
	- purejavahidapi.jar from https://github.com/nyholku/purejavahidapi/tree/master/bin
	- xchart-xxx.jat from https://knowm.org/open-source/xchart/xchart-change-log/
	+ then add to build path; in eclipse: right click on project->Properties->Java build path->Libraries->Classpath delete all existing then click AddJARs to select copied files
- build, run and test. Linux usually have no permission to device files, try run as root: 'chmod 777 /dev/hidraw*' AFTER connecting USB to be sure.

