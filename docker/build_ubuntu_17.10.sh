#!/bin/sh

docker build -t martin/qrsspigdev:ubuntu17.10 -f docker/Dockerfile_build_Ubuntu_17.10 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu17.10 bash -c "mkdir -p build/ubuntu17.10 && cd build/ubuntu17.10 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
