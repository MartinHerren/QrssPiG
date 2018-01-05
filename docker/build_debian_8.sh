#!/bin/sh

docker build -t martin/qrsspigdev:debian8 -f docker/Dockerfile_build_Debian_8 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:debian8 bash -c "mkdir -p build/debian8 && cd build/debian8 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
