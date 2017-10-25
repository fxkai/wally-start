# Overview

The wally daemon is a node based daemon used to control wally devices right after start. Its functionality mainly is

* display startup informations in a very early stage of a device startup (written in C)
* control peripherie of the wally device (i.e. touch panel, inputs, rfid, gpio, sensors ... written in NodeJS)
* display generic data locally acquired by the device
* render remote data (such as webpages) to the display
* render remote data from a middleware to the display

Projects Wally (TV,Cam,ID,Photobooth) are used in

* customers info panels driven by the FreshX Wallaby backend
* static info panels
* Wally Photobooth
* CO2/Humidity sensor+display
* Identity devices 

# Hardware supported

* Raspberry Pi 1/2/3
* Linux devices running DRM/KMS (ATI,Nvidia, Intel, VMWare)
* Mac OS X
* (Windows, not tested)

Requirements

* SDL2 SDL2_image SDL2_ttf SDL2_gfx
* for linux : libdrm, libdri2, libgbm
* the video plugin requires ffmpeg-devel

