#!/bin/bash
PROJECT_SRC=$(cd `dirname $0`; pwd)
PROJECT_DIR_NAME="${PROJECT_SRC##*/}"
PROJECT=${PROJECT_DIR_NAME}

# touch ${PROJECT}.kdev4
add_kdev4()
{
    echo "[Project]" 2>&1 | tee ${PROJECT}.kdev4
    echo "Manager=KDevCMakeManager" >> ${PROJECT}.kdev4
    echo "Name=${PROJECT}" >> ${PROJECT}.kdev4
}
add_kdev4;

mkdir -p build && cd build

rm -rf *

cmake ../

make
cp -rf $PROJECT ../

file $PROJECT
