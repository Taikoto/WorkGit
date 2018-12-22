#!/bin/bash
./eiot/tools/load.sh $*
project=$(cat mtkbase)
./eiot/tools/prebuild.sh
. build/envsetup.sh
lunch full_${project}-userdebug
make -j24 2>&1 | tee build.log
#./eiot/tools/imageout.sh
#./eiot/tools/zip.sh