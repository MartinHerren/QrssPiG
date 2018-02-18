#!/bin/sh

docker build -t martin/qrsspigdev:ubuntu14.04 -f docker/Dockerfile_build_Ubuntu_14.04 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu14.04 bash -c "mkdir -p build/ubuntu14.04 && cd build/ubuntu14.04 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
