# QrssPiG

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)
[![Build Status](https://travis-ci.org/MartinHerren/QrssPiG.svg?branch=dev)](https://travis-ci.org/MartinHerren/QrssPiG)

QrssPiG is short for QRSS (Raspberry)Pi Grabber.

Haven't found a headless standalone grabber for my Pi. So I try to create my own.

## Functionality
 - Headless QRSS grabber
 - Process I/Q stream from an rtl-sdr or HackRF device
 - Process I/Q stream from a stereo audio input or an audio stream from a mono audio input. Currently only 16 bit audio supported
 - Generate pretty horizontal or vertical waterfall graphs
 - Upload them via scp or ftp, or just save locally. Or any combination of uploads and local saves

## Sample Output
<img src="https://www.dxzone.com/dx32901/qrsspig.jpg">

## Installation
### Install from Debian 9 repository
There is a Debian Stretch (Debian 9) repository with binaries for amd64 and armhf (Raspberry). To add the repository:
Create a file /etc/apt/sources.list.d/hb9fxx.list containing
```
deb https://debian.hb9fxx.ch/debian/ dev/
deb-src https://debian.hb9fxx.ch/debian/ dev/
```
and run
```
sudo aptitude install apt-transport-https
wget https://debian.hb9fxx.ch/debian/key.asc -O - | sudo apt-key add -
sudo aptitude update
sudo aptitude install qrsspig
```

### Install from SuSE repository
Thanks to Martin Hauke there are SuSE binaries available. For this you'll need to add the hardware:sdr repository.
e.g. for openSUSE Leap 42.3:
```
$ sudo zypper addrepo -f
https://download.opensuse.org/repositories/hardware:/sdr/openSUSE_Leap_42.3/hardware:sdr.repo
$ sudo zypper install QrssPiG
```

### Build from source
#### Dependencies
To build QrssPiG you need cmake and a c++ compiler (g++ or clang++) with support for c++11.

To build the core of QrssPiG you need the following mandatory dev libs:
 - libboost-program-options-dev
 - libyaml-cpp-dev
 - libfftw3-dev
 - libgd-dev

Without any additional libs QrssPiG is limited to read data from its standard input and save the output to the local filesystem.

Additional functionalities are available through following libs:
 - librtlsdr-dev: Read stream from rtlsdr device
 - libhackrf-dev: Read stream from hackrf device
 - libasound2-dev: Read stream from alsa audio device
 - libssh-dev: SCP upload of grabs to a server
 - libcurl4-openssl-dev: FTP upload of grabs to a server
 - libliquid-dev: Downsampling of input signal samplerate before processing
 - librtfilter-dev: Alternative for libliquid-dev

#### Get the sources
Clone the git repository from https://github.com/MartinHerren/QrssPiG.git

or download the latest snapshot from https://github.com/MartinHerren/QrssPiG/archive/master.zip
To have access to the latest developments you can use the dev branch instead of the master.

#### Build:
Inside the QrssPiG directory:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

## Running QrssPiG
To get a short help about QrssPiG's usage, type
```
$ qrsspig -h
```

To get the list of input/upload modules QrssPiG has been compiled with, type
```
$ qrsspig -m
```

To list found input devices, type
```
$ qrsspig -l
```

You can combine the options -m and -l:
```
$ qrsspig -ml
```
To run QrssPiG, type:
```
$ qrsspig -c configfile.yaml
```
alternatively you can omit the '-c' and just give only the config file
```
$ qrsspig configfile.yaml
```

A simple template for a config file is contained in the git repository: qrsspig.yaml.template, some simple example are given in the next section. For a complete documentation of all options in the config file, refer to the wiki.

## Example configs
### Simple acquisition from a mono soundcard:
This is a typical config for a RaspberryPi with a cheap external mono USB sound dongle (almost all the cheap ones only feature a mono input and a stereo output, sold as 7.1 output). This kind of config is even able to run on the simple first generation RaspberryPi 1 with a single core running at 700MHz and only 256MB of RAM. It can also run on a ReapberryPi Zero(W).
```
input:
  type: alsa
  device: hw:1
  channel: mono
  samplerate: 48000
  basefreq: 10138500
processing:
  fft: 65536
  fftoverlap: 1
output:
  orientation: horizontal
  minutesperframe: 10
  freqmin: 1000
  freqmax: 2000
  dBmin: -50
  dBmax: -20
upload:
  type: local
  dir: /tmp
```

### Simple acquisition from an rtlsdr dongle
This is a typical config for a RaspberryPi with a cheap rtl-sdr dongle. This config is able to run on RaspberryPi 2/3.
```
input:
  type: rtlsdr
  samplerate: 240000
  basefreq: 27999300
  gain: 24
processing:
  fft: 262144
  fftoverlap: 1
output:
  orientation: horizontal
  minutesperframe: 10
  freqmin: 1000
  freqmax: 2000
  dBmin: -50
  dBmax: -20
upload:
  type: local
  dir: /tmp
```

### Simple acquisition from an HackRF
This config involves downsampling from 8MS/s to 8kS/s so it is CPU intensive and needs a PC to run.
```
input:
  type: hackrf
  samplerate: 8000000
  basefreq: 27999300
  amplifier: on
  lnagain: 24
  vgagain: 32
processing:
  samplerate: 8000
  fft: 8192
  fftoverlap: 1
output:
  orientation: horizontal
  minutesperframe: 10
  freqmin: 1000
  freqmax: 2000
  dBmin: -50
  dBmax: -20
upload:
  type: local
  dir: /tmp
```
