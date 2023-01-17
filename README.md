# VBIT2

An installation guide and more can be found in the [github wiki](https://github.com/peterkvt80/vbit2/wiki).

## About

This program takes a set of teletext [page files](https://github.com/peterkvt80/vbit2/wiki/Page-files) and generates a feature rich transmission stream on stdout.

The transmission stream can be piped to raspi-teletext or any other application that needs a teletext packet stream.
It is a console application that can be compiled for Raspberry Pi or Windows.

It generates a T42 teletext stream that can be piped to [raspi-teletext](https://github.com/ali1234/raspi-teletext) to add a teletext signal to the Raspberry Pi composite output, [vbit-py](https://github.com/peterkvt80/vbit-py) to drive a Vbit teletext inserter board, or into the [vbit-iv](https://github.com/peterkvt80/vbit-iv) in-vision renderer.

VBIT2 can also optionally generate output a Packetized Elementary Stream for insertion into an MPEG transport stream for DVB teletext.

## Features

VBIT2 includes the following features:
* Parallel mode transmission of teletext magazines.
* Cycling subpage carousels.
* Level 1.5 and 2.5 features including dynamic character and object downloading pages and Packet 29 insertion.
* Format 1 Broadcast Service Data Packet generation with automatic daylight saving time.
* Fastext and TOP navigation support.
