# Overview

The wally daemon is a node based daemon used to control wally devices right after start. Its functionality mainly is

* display startup informations in a very early stage of a device startup (written in C)
* display generic data locally acquired by the device

# Hardware supported

* Raspberry Pi 1/2/3
* Linux devices running X11 (ATI,Nvidia, Intel, VMWare)
* Mac OS X

Requirements

* SDL2 SDL2_image SDL2_ttf SDL2_gfx
* for linux : sdl2 gfx bindings (opengl, X11 or Pi libs)
* cmake

# Build

```bash
cmake .
make && make install
```
