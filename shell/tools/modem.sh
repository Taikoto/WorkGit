#!/bin/bash
#项目
projectConfig=$1
#平台
platform=$2

mkdir -p  ./vendor/mediatek/proprietary/modem/
cp ./device/mediatek/build/build/tools/modem/modem_Android.mk  ./vendor/mediatek/proprietary/modem/Android.mk
echo ===============================$projectConfig
modems=`cat ${projectConfig} |grep CUSTOM_MODEM|awk -F"=" '{print $2}'`
echo ===============================$modems
for item in $modems
do
echo ===============$item;
cp -ar ./eiot/modems/$platform/$item  ./vendor/mediatek/proprietary/modem/
done

