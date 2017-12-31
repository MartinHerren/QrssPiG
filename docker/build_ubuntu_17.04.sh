#!/bin/sh

docker build -t martin/qrsspigdev:ubuntu17.04 -f docker/Dockerfile_build_Ubuntu_17.04 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu17.04 bash -c "mkdir -p build/ubuntu17.04 && cd build/ubuntu17.04 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
