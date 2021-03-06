# vbit2

An installation guide and more can be found in the [wiki](https://github.com/peterkvt80/vbit2/wiki).

## About VBIT

Teletext streaming. This takes a set up teletext files and generates a transmission stream on stdout.
The transmission stream can be fed into raspi-teletext or any other application that needs a teletext packet stream.
This is intended to replace vbit-pi-stream. It handles carousels better and uses ram to buffer pages rather than keeping them on file.
It will use a lot more ram but also a lot less file system access and less CPU.
It is a console application that can be compiled for Raspberry Pi or Windows.

It generates a T42 teletext stream that can be piped into raspi-teletext to make teletext that TVs can decode. Or into vbit-py to drive a vbit2 teletext inserter. Or into the vbit-iv in-vision renderer.

This program is written in c++11. It was developed on Windows. I used Code::Blocks and added a compiler TDM-GCC-64.
The toolchain change is needed to support c++11 and more specifically threads.
Please contact me if you are trying to set up a windows development environment.

On Raspberry Pi it uses the default development environment. Just changed CFLAGS to CXXFLAGS and added flag -std=gnu++11

vbit2 probably outputs a lot of garbage on cerr depending on how I left it.
You only want cout.

To get only cout run vbit2 like this

./vbit2 2> /dev/null

or on Windows

vbit2.exe 2> NUL
