#!/bin/sh

for dist in "$@"
do
	docker build -t martin/qrsspigdev:$dist -f docker/Dockerfile_build_$dist docker/
done
