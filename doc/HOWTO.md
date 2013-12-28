#GNFC
GNFC is an OpenSource GUI NFC tool, it is based on QT and several nfc libraries(libnfc/libfreefare/libllcp/libndef). GNFC aims at showing how to NFC devices/modules to read card, write card, even exchange data with Android 4.0+ device.  


## Supported Devices

### [Elechosue USB NFC Device](http://elechouse.com)

[ELECHOUSE USB NFC Device]() is based on PN532, with an MCU and a beeper inside, the beeper can be controlled by sending command(in PN532 frame format) to it, the command value to control beeper is **0xA0**, after receive the `BEEP` command [ELECHOUSE USB NFC Device](http://elechouse.com) won't send ack package.

### [Elechosue WIFI NFC Device](http://elechouse.com) 
It is an OpenWrt based device, with a pn532 module in it. It is supported by pn532_net driver, pn532_net driver is at [JiapengLi/libnfc](https://github.com/JiapengLi/libnfc)

[Elechosue WIFI NFC Device]() 

## Dependence

+ [libnfc](https://code.google.com/p/libnfc/)
+ [libfreefare](https://code.google.com/p/libfreefare/)
+ [libllcp](https://code.google.com/p/libllcp/)
+ [libndef](https://github.com/nfc-tools/libndef)
+ [QT5](http://qt-project.org/)

## Compile Environment

### Windows

There is a pre-build binary file. [Download](https://github.com/JiapengLi/GNFC/releases/download/v0.2.0/gnfc-0.2.0-win.zip)

#### Tools
Some tools is needed to compile libnfc and libfreefare.  

+ [Win32OpenSSL](http://slproweb.com/download/Win32OpenSSL-1_0_1e.exe)
+ [PCRE](http://hivelocity.dl.sourceforge.net/project/gnuwin32/pcre/7.0/pcre-7.0.exe)
+ [cmake](http://www.cmake.org/files/v2.8/cmake-2.8.12.1-win32-x86.exe)

Install all of these tools.

#### QT5
Install [QT5.1.1](http://download.qt-project.org/official_releases/qt/) or high.

To build libnfc and libfreefare, we use the MinGW tool chain of QT, so add
`Qt/Qt5.1.1/Tools/mingw48_32/bin` to environment variable `PATH`

#### libnfc
[A detailed instruction to compile libnfc under Windows](http://www.mobilefish.com/developer/libnfc/libnfc.html)
![0](./img/00-configure-libnfc.jpg)

#### libfreefare
Use cmake to configure libfreefare,  then generate Makefile for it.
![1](./img/01-configure-libfreefare.jpg)

![2](./img/02-configure-libfreefare.jpg)

#### libllcp
Download the libllcp source code. switch to **origin/socket** branch. Then use cmake to generate MinGW Makefiles just like *libfreefare*, by addition, specify the libws2_32.lib.a for it.

	mingw32-make

#### libndef
	

*Install programs by default and set parameter like pictures above may save some time.* 

### Linux

#### QT5

1. Download [`qt-linux-opensource-5.x.x-x86-offline.run`](http://download.qt-project.org/official_releases/qt/)
2. `sudo chmod +x qt-linux-opensource-5.1.1-x86-offline.run`
3. `./qt-linux-opensource-5.1.1-x86-offline.run`
4. next...next...next...
5. (Optional) Fix startup error. [See Reference](http://askubuntu.com/questions/253785/cannot-overwrite-file-home-baadshah-config-qtproject-qtcreator-toolchains-xml)

        sudo -s chmod o+w /home/yourname/.config/QtProject/qtcreator/*.x
        sudo chown -R $USER:$USER /home/lich/.config/QtProject/

#### libnfc

*so far, pn532_net driver is not included by libnfc, checkout [JiapengLi/libnfc](https://github.com/JiapengLi/libnfc) instead, by which net driver is included. To use net driver specify a device name like "pn532_net:192.168.1.1:8000".*

	autoreconf -vis
	./configure --with-drivers=pn532_uart,pn532_net --sysconfdir=/etc --prefix=/usr
	make
	sudo make install all

#### libfreefare
	
	autoreconf -vis
	./configure --prefix=/usr
	make
	sudo make install

#### libllcp
	
	autoreconf -vis
	./configure --prefix=/usr
	make
	sudo make install

#### libndef
	
	qmake
	mingw32-make

lib files locate at *libndef/libndef/release/*, named **libndef1.a** and **ndef1.dll**, libndef/include/*.h is also needed.


## GNFC
After all are prepared done. Download the GNFC source code, open the `gnfc.pro` with QT Creator, compile and run it.

### Windows
For windows, the dll files below may be needed. Different QT may be different here.

	./icudt51.dll
	./icuin51.dll
	./icuuc51.dll
	./libeay32.dll
	./libfreefare.dll
	./libgcc_s_dw2-1.dll
	./libnfc.dll
	./libstdc++-6.dll
	./libwinpthread-1.dll
	./pcre3.dll
	./Qt5Core.dll
	./Qt5Gui.dll
	./Qt5SerialPort.dll
	./Qt5Widgets.dll

	./platforms/qwindows.dll

### Linux