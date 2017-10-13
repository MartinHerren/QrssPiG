#!/bin/sh
set -ex
wget https://github.com/mossmann/hackrf/archive/v2015.07.2.tar.gz
tar -xzvf v2015.07.2.tar.gz
mkdir -p hackrf-2015.07.2/host/build && cd hackrf-2015.07.2/host/build && cmake ../ && make && sudo make install && sudo ldconfig
