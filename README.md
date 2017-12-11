# QrssPiG

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)
[![Build Status](https://travis-ci.org/MartinHerren/QrssPiG.svg?branch=dev)](https://travis-ci.org/MartinHerren/QrssPiG)

QrssPiG is short for QRSS (Raspberry)Pi Grabber.

Haven't found a headless standalone grabber for my Pi. So I try to create my own.
If i find one working for my needs I might well using the existing one and stop this.

## Functionality
 - Headless standalone daemon
 - Able to process I/Q stream from an rtl-sdr, HackRF or other sdr devices
 - Optionally control the sdr device
 - Optionally process audio from stream or audio input
 - Generate pretty horizontal or vertical waterfall graphs
 - Upload them via scp or ftp, or just save locally. Or any combination of uploads and local saves

## Install from Debian repository
There is a Debian Strech (Debian 9) repository with binaries for amd64 and armhf (Raspberry). To add the repository:
Create a file /etc/apt/sources.list.d/hb9fxx.list containing
```
deb https://debian.hb9fxx.ch/debian/ dev/
deb-src https://debian.hb9fxx.ch/debian/ dev/
```
and run
```
sudo aptitude install apt-transport-https
wget https://debian.hb9fxx.ch/debian/key.asc -O - | sudo apt-key add
sudo aptitude update
sudo aptitude install qrsspig
```

You now can run qrsspig giving it a config file as shown below in the run section.

## Install from SuSE repository
Thanks to Martin Hauke there are SuSE binaries available. For this you'll need to add the hardware:sdr repository.
e.g. for openSUSE Leap 42.3:
```
$ sudo zypper addrepo -f
https://download.opensuse.org/repositories/hardware:/sdr/openSUSE_Leap_42.3/hardware:sdr.repo
$ sudo zypper install QrssPiG
```

## Build
To build QrssPiG you need cmake

The build depends on following mandatory dev libs:
 - libboost-program-options-dev
 - libyaml-cpp-dev
 - libfftw3-dev
 - libgd-dev
 - libfreetype6-dev

Additional functionalities are available through following libs:
 - libssh-dev: SCP upload of grabs to a server
 - libcurl4-openssl-dev: FTP upload of grabs to a server
 - libliquid-dev: Downsampling of input signal samplerate before processing
 - librtfilter-dev: Alternative for libliquid-dev

To build:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Run
### Piping from audio device
You need arecord to be installed. From your build directory
```
arecord -q -D hw:1 -t raw -f S16_LE -r 48000 -c 1 --buffer-size=48000 | ./src/qrsspig -c qrss.yaml
```
arecord is used to record from the second audio input (audio usb dongle with mono input) as raw audio data without header in signed 16 bit little-endian format with 48kHz samplerate. Output is sent to standard out. 1 second buffer to prevent overflow.
A receiver tuned to 10138500Hz must be recorded to the audio input. The frequency is choosen so that with a frequency accurate receiver qrss data should appear around 1.5kHz which will be in the middle of the plot.
In the qrss.yaml config file you must set the sample rate to 6000 and the format to s16iq like in the following example:
```
input:
  format: s16mono
  samplerate: 48000
  basefreq: 10138500
processing:
  samplerate: 6000
  fft: 16384
  fftoverlap: 3
output:
  orientation: horizontal
  minutesperframe: 10
  freqmin: 300
  freqmax: 2700
  dBmin: -30
  dBmax: 0
upload:
  type: local
```
If your soundcard supports stereo input you must specify -c 2 instead of -c 1 for arecord and use s16left or s16right for format in the config file, depending on which channel of the stereo input your radio is connected.

### Piping from rtl_sdr
You need rtl_sdr installed. From your build directory
```
rtl_sdr -f 27999300 -s 240000 -g 60 - | ./src/qrsspig -c qrss.yaml
```
rtl_sdr is used as receiver and produces unsigned 8 bit I/Q data at a samplerate of 240kSample/s from 27999300Hz and send them to stdout.
The frequency is choosen so that with a frequency accurate receiver qrss data should appear around 1.5kHz which will be in the middle of the plot.
In the qrss.yaml config file you must set the sample rate to the input samplerate of 240000 and a processing samplerate of 6000 and the format to rtlsdr (or u8iq) like in the following example:
```
input:
  format: rtlsdr
  samplerate: 240000
  basefreq: 27999300
processing:
  samplerate: 6000
  fft: 16384
  fftoverlap: 3
output:
  title: QrssPiG 10m QRSS Grabber
  orientation: horizontal
  minutesperframe: 10
  freqmin: 300
  freqmax: 2700
  dBmin: -40
  dBmax: -15
upload:
  type: local
```

### Piping from hackrf through sox for resampling
You need a recent version of hackrf_transfer (first version couldn't stream to stdout) and sox installed. From your build directory
```
hackrf_transfer -f 10138500 -s 8000000 -b 1.75 -a 1 -g 60 -l 24 -r - | sox -t s8 -c 2 -r 8000000 - -t s8 -c 2 -r 6000 - | ./src/qrsspig -c qrss.yaml
```
hackrf_transfer is used as receiver and produces signed 8 bit I/Q data at a samplerate of 8MSample/s from 10138500Hz and send them to stdout.
The frequency is choosen so that with a frequency accurate receiver qrss data should appear around 1.5kHz which will be in the middle of the plot.
sox is used to resample the data from 8MSample/s to 6kSample/s. Internal downsampler doesn't work yet for hackrf_transfer.
In the qrss.yaml config file you must set the sample rate to 6000 and the format to hackrf (or s8iq) like in the following example:
```
input:
  format: hackrf
  samplerate: 6000
  basefreq: 10138500
processing:
  fft: 16384
  fftoverlap: 3
output:
  orientation: horizontal
  minutesperframe: 10
  freqmin: 300
  freqmax: 2700
  dBmin: -40
  dBmax: -30
upload:
  type: local
```
