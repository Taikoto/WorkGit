#!/bin/bash

PROJECT_NAME=$1
BUILD_TYPE=$2
AP_DIR=$(cd `dirname $0` ; pwd)
BOARD_PATH=$(find $AP_DIR/device/sprd -name "${PROJECT_NAME}.mk"|xargs dirname)
echo $BOARD_PATH
BOARD_NAME=${BOARD_PATH##*/}
OUT_DIR="$AP_DIR/out/target/product/$BOARD_NAME"
DATE_DIR=$(date +%Y%m%d%H%M)
RELEASE_DIR=$AP_DIR/release-sp9820e_s01
PAC_DIR="$AP_DIR/vendor/sprd/release/PAC_Resource/Scripts"

if [ ! -d "$RELEASE_DIR" ]; then
   echo $RELEASE_DIR 不存在，新建一个
   mkdir $RELEASE_DIR
fi

case $PROJECT_NAME in
sp9820e_2h10_oversea)
echo "start to sign modem..."
pac_files="
$RELEASE_DIR/${PROJECT_NAME}_${BUILD_TYPE}_${DATE_DIR}.pac
sp9820e_2h10
MOCOR5_sfphone
$OUT_DIR/sp9820e_2h10.xml
$OUT_DIR/fdl1-sign.bin
$OUT_DIR/fdl2-sign.bin
$OUT_DIR/sharkle_pubcp_customer_Feature_Phone_nvitem.bin
$OUT_DIR/prodnv_b256k_p4k.img
$OUT_DIR/u-boot-spl-16k-sign.bin
$OUT_DIR/SC9600_sharkle_pubcp_customer_Feature_Phone_modem.dat
$OUT_DIR/SharkLE_LTEA_DSP_evs_off.bin
$OUT_DIR/SHARKLE1_DM_DSP.bin
$OUT_DIR/sharkle_cm4.bin 
$OUT_DIR/PM_sharkle_cm4.bin
$OUT_DIR/boot.img
$OUT_DIR/recovery.img
$OUT_DIR/system_b256k_p4k.img
$OUT_DIR/userdata_b256k_p4k.img
$OUT_DIR/sprd_320240.bmp
$OUT_DIR/sprd_320240.bmp
$OUT_DIR/cache_b256k_p4k.img
$OUT_DIR/u-boot-sign.bin
$OUT_DIR/sharkle_pubcp_customer_Feature_Phone_deltanv.bin
$OUT_DIR/sml-sign.bin
$OUT_DIR/tos-sign.bin
$OUT_DIR/gnssbdmodem.bin
$OUT_DIR/gnssmodem.bin
$OUT_DIR/prodnv_b128k_p2k.img
$OUT_DIR/system_b128k_p2k.img
$OUT_DIR/userdata_b128k_p2k.img
$OUT_DIR/cache_b128k_p2k.img
"

perl $PAC_DIR/pac_via_conf.pl  $pac_files
echo sp9820e_2h10_oversea
;;
sp9820e_1h10_oversea)
echo "start to sign modem..."
pac_files="
$RELEASE_DIR/${PROJECT_NAME}_${BUILD_TYPE}_${DATE_DIR}.pac
sp9820e_1h10
MOCOR5_sfphone
$OUT_DIR/sp9820e_1h10.xml
$OUT_DIR/fdl1-sign.bin
$OUT_DIR/fdl2-sign.bin
$OUT_DIR/sharkle_pubcp_customer_Feature_Phone_nvitem.bin
$OUT_DIR/prodnv.img
$OUT_DIR/u-boot-spl-16k-sign.bin
$OUT_DIR/SC9600_sharkle_pubcp_customer_Feature_Phone_modem.dat
$OUT_DIR/SharkLE_LTEA_DSP_evs_off.bin
$OUT_DIR/SHARKLE1_DM_DSP.bin
$OUT_DIR/sharkle_cm4.bin 
$OUT_DIR/PM_sharkle_cm4.bin
$OUT_DIR/boot.img
$OUT_DIR/recovery.img
$OUT_DIR/system.img
$OUT_DIR/userdata.img
$OUT_DIR/sprd_320240.bmp
$OUT_DIR/sprd_320240.bmp
$OUT_DIR/cache.img
$OUT_DIR/u-boot-sign.bin
$OUT_DIR/sharkle_pubcp_customer_Feature_Phone_deltanv.bin
$OUT_DIR/sml-sign.bin
$OUT_DIR/tos-sign.bin
$OUT_DIR/gnssbdmodem.bin
$OUT_DIR/gnssmodem.bin
"

perl $PAC_DIR/pac_emmc.pl  $pac_files
echo sp9820e_1h10_oversea
;;
*) 
  echo  "check your  input  project name !!!!"
esac
