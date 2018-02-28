#!/bin/sh

version=0.5.1

for dist in "$@"
do
	docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:$dist bash -c "./docker/debuild_binary_host.sh $version $dist"
done
