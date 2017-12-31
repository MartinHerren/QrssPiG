#!/bin/sh

docker build -t martin/qrsspigdev:debianbuster -f docker/Dockerfile_build_Debian_Buster docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:debianbuster bash -c "mkdir -p build/debianbuster && cd build/debianbuster && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
