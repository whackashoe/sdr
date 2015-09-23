#!/bin/sh

cd db/server
./build.sh

cd ../cli
./build.sh

cd ../../benchmark
./build.sh