#!/bin/sh

docker build -t martin/qrsspigdev:ubuntu18.04 -f docker/Dockerfile_build_Ubuntu_18.04 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu18.04 bash -c "mkdir -p build/ubuntu18.04 && cd build/ubuntu18.04 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
