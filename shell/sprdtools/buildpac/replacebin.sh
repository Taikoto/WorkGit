#!/bin/bash

REPLACEBIN=$ANDROID_BUILD_SRC/vendor/sprd/release/basePac/replacebin/$ModemVersion
echo $REPLACEBIN
cp -rf $REPLACEBIN/* $OUT_TARGET || return 1
echo "copy modem file finnished !"
