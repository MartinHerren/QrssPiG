#!/bin/sh

dist=$1

docker build -t martin/qrsspigdev:$dist -f docker/Dockerfile_build_$dist docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:$dist bash -c "mkdir -p build/$dist && cd build/$dist && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
