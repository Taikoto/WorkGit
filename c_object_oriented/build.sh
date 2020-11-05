#!/bin/bash
PROJECT_SRC=$(cd `dirname $0`; pwd)
PROJECT_DIR_NAME="${PROJECT_SRC##*/}"
PROJECT=${PROJECT_DIR_NAME}

mkdir -p build && cd build

rm -rf *

cmake ../

make
cp -rf $PROJECT ../

file $PROJECT
