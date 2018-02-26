#!/bin/sh

for dist in "$@"
do
	docker run --rm -ti -v $PWD:$PWD -w $PWD martin/qrsspigdev:$dist bash -c "mkdir -p build/$dist && cd build/$dist && cmake ../.. && cmake --build . && ./src/qrsspig -ml"
done
