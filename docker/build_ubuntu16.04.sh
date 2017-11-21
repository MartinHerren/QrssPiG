#!/bin/sh

docker build -t martin/qrsspigdev:ubuntu16.04 -f docker/Dockerfile_build_Ubuntu16.04 docker/
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu16.04 bash -c "mkdir -p build/ubuntu16.04 && cd build/ubuntu16.04 && cmake ../.. && cmake --build ."
docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:ubuntu16.04 bash -c "./build/ubuntu16.04/src/qrsspig -l"
