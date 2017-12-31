#!/bin/sh

docker build -t martin/qrsspigdev:ubuntu16.10 -f docker/Dockerfile_build_Ubuntu_16.10 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu16.10 bash -c "mkdir -p build/ubuntu16.10 && cd build/ubuntu16.10 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
