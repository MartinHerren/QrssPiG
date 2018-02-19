#!/bin/sh

docker build -t martin/qrsspigdev:debian10 -f docker/Dockerfile_build_Debian_10 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:debian10 bash -c "mkdir -p build/debian10 && cd build/debian10 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
