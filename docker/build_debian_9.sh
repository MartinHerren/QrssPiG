#!/bin/sh

docker build -t martin/qrsspigdev:debian9 -f docker/Dockerfile_build_Debian_9 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:debian9 bash -c "mkdir -p build/debian9 && cd build/debian9 && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
