#!/bin/bash

export srcpath=$(cd `dirname $0`; pwd)
export ANDROID_BUILD_SRC=$(cd `dirname $0`; pwd)
export VENDOR_IDH_TARGET=$ANDROID_BUILD_SRC/vendor/sprd/release/IDH
export VENDOR_TEST_TARGET=$ANDROID_BUILD_SRC/vendor/sprd/release/test
export VENDOR_OUT_TARGET=$VENDOR_TEST_TARGET/out
source ./build/setjavaversion.sh
date

ParmsNum=6
iniFile="./build.ini"		# 编译参数记忆文件
BOARDINFO="./Boardinfo.ini"

function printHelp()
{
	echo "====================================================="
	echo "= 帮助 =============================================="
	echo "====================================================="
	echo "【方式1】 autobuild.sh 项目 类型 OTA 全清除 发布版本 延时编译时间 [更新代码]"
	echo 参数说明:
	echo 参数1:要编译的项目名字，比如 G5_320_320 G5_240_240 等等
	echo 参数2:要编译的类型,比如 user userdebug eng
	echo 参数3:是否编译带OTA功能的软件? y/n 
	echo 参数4:要全部clean吗? y/n
	echo 参数5:要Release发布版本吗? y/n
	echo 参数6:要多少分钟之后开始编译? 180m 60m 10m 0m 等等	
	echo "参数7:更新Git代码? y/n(可选, 省略此参数, 则默认为 y)"
	echo
	echo "示例:"
	echo "./autobuild.sh 项目名称 user y y y 180m n"
	echo
	echo
	echo "【方式2】 autobuild.sh 编译方法 [更新代码]"
	echo "如果发现有 $iniFile 文件，而会读取文件中的编译参数进行编译"
	echo
	echo "参数说明:"
	echo "编译方法: new 或 remake 编译(n / r)"
	echo "更新代码: 从 Git 库中更新代码(y/n), 可选, 省略此参数, 则默认为 n"
	echo
	echo "示例："
	echo "new 编译:"
	echo "./autobuild.sh n"
	echo
	echo "remake 编译"
	echo "./autobuild.sh r"
	echo "====================================================="
	echo
}


function printHelpTips()
{
	echo "帮助: ./autobuild.sh -h"
}

function printParms_2()
{
    echo "=== 检查编译参数 ==="
    echo "项目:     $PROJECTNAME"
    echo "编译类型: $TARGET_BUILD_VARIANT"
    echo "编译OTA:  $IS_OTA"
    echo "new 编译: $BUILDNEW"
    echo "发布版本: $RELEASESoftware"
    echo "多少分钟后编译: $SLEEPTIME"
    echo "更新代码: $UPDATE_CODE"
	echo
}

VARIANT_CHOICES=(user userdebug eng)

function check_variant()
{
    for v in ${VARIANT_CHOICES[@]}
    do
        if [ "$v" = "$1" ] ; then
            return 1
        fi
    done
    return 0
}


# 如果参数 >= 0, <= 2 ，则考虑使用 Ini 文件的参数
if [ $# -ge 1 -a $# -le 2 ] ; then
	if [ $# -ge 1 -a $1 = "-h" ] ;then
		printHelp
	fi

	if [ -f "$iniFile" ]; then
		source $iniFile
		SLEEPTIME="0m"
		
		# 以输入的参数替换文件的编译方式
		buildNewRemake=$( echo $1 | tr A-Z a-z )
		if [ $buildNewRemake == "n" -o $buildNewRemake == "new" ] ; then
			BUILDNEW="y"
		elif [ $buildNewRemake == "r" -o $buildNewRemake == "remake" ] ; then
				BUILDNEW="n"
			else
			echo "*** 参数不正确! 编译方法只能是: new|remake|n|r"
			printHelpTips
				exit;
			fi

		# 默认不更新 Git 代码
		UPDATE_CODE=n	
		if [ $# -gt 1 ] ;then
			UPDATE_CODE=$2
		fi
		
		# 检查参数是否有效
		InvalidArgument=0
		InvalidVeriant=0
		if test -z "$PROJECTNAME"; then
			InvalidArgument=1
		elif test -z "$TARGET_BUILD_VARIANT"; then
			InvalidArgument=1
		elif check_variant $TARGET_BUILD_VARIANT ; then
			InvalidVeriant=1
			InvalidArgument=1
		elif test -z "$IS_OTA"; then
			InvalidArgument=1
		elif test -z "$BUILDNEW"; then
			InvalidArgument=1
		elif test -z "$RELEASESoftware"; then
			InvalidArgument=1
		elif test -z "$SLEEPTIME"; then
			InvalidArgument=1
		fi

		if [ $InvalidArgument -ne 0 ] ;then
			echo "*** 配置文件参数不正确! 配置文件:$iniFile"
 
			# 检查编译类型参数
			if [ $InvalidVeriant -ne 0 ]; then
				echo "编译类型只能是: user|userdebug|eng"
			else
				# 打印参数
				printParms_2
			fi
			printHelpTips
			exit;
		fi
	else
		echo "*** 编译配置文件不存! $iniFile"
		exit;
	fi
elif [ $# = 0 ] ;then
		printHelp
		exit;
	else
		PROJECTNAME=$1
		TARGET_BUILD_VARIANT=$2
		IS_OTA=$3
	BUILDNEW=$4
	RELEASESoftware=$5
	SLEEPTIME=$6
		
	# 默认更新 Git 代码
	UPDATE_CODE=y
	if [ $# -gt $ParmsNum ] ;then
		UPDATE_CODE=$7
	fi
 
	if [ $# -lt $ParmsNum ] ;then
		# 打印参数
		printParms_2
		echo "*** 参数太少!"
		printHelpTips
		exit;
	fi
fi

# 打印参数
printParms_2

error=0

# 检查项目是否存在
projPath="./project/$PROJECTNAME"
if [ ! -d "$projPath" ]; then  
	echo "*** 项目不存在! $projPath"
	error=1
fi

# 检查编译类型参数
if check_variant $TARGET_BUILD_VARIANT ; then
	echo "*** 编译类型只能是: user|userdebug|eng"
	error=1
fi

if [ $error = 1 ] ; then
	exit;
fi

echo -n "确认参数无误, 并开始执行？【回车】确认, 【任意键】取消："
read flag
if [ "$flag" = "" ] ; then
	echo
else
	echo "已取消执行!"
	exit
fi

# 保存参数到文件
echo PROJECTNAME=$PROJECTNAME > $iniFile
echo TARGET_BUILD_VARIANT=$TARGET_BUILD_VARIANT >> $iniFile
echo IS_OTA=$IS_OTA >> $iniFile
echo RELEASESoftware=$RELEASESoftware >> $iniFile

if [[ $debug = 1 ]]; then
	echo "#### 调试结束! ####"
	exit
fi


#####################更新代码######################
if [ "$UPDATE_CODE" = "y" -o "$UPDATE_CODE" = "yes" ] ;then
	date
	#更新代码
	echo "先更新代码"
	git pull
	sleep 10s
	echo "更新完毕"
	sleep 5s
	#再延时操作
	#sleep $SLEEPTIME
fi
 ######################################################


cd $ANDROID_BUILD_SRC/project
./projectdelnew.sh $PROJECTNAME
#回到当前目录
cd $ANDROID_BUILD_SRC
ProjectName="default"
ProjectName=$(cat project/ProjectName.txt)
echo ProjectName=$ProjectName
#rm -rf project/ProjectName.txt
ConfigFile=project/${ProjectName}/config.txt
echo ConfigFile=$ConfigFile
source $ConfigFile

if [[ $TARGET_PRODUCT == "" ]]; then
    TARGET_PRODUCT="sp9820e_2h10_oversea"
fi
echo TARGET_PRODUCT=$TARGET_PRODUCT

echo 项目客制化已经完成，将会延时$SLEEPTIME后开始真正编译
sleep $SLEEPTIME
BOARD=$TARGET_PRODUCT-$TARGET_BUILD_VARIANT
echo $BOARD > $BOARDINFO

source build/envsetup.sh;
lunch $TARGET_PRODUCT-$TARGET_BUILD_VARIANT

export TARGET_PRODUCT=$TARGET_PRODUCT
export TARGET_BUILD_VARIANT=$TARGET_BUILD_VARIANT

if [ "$BUILDNEW" = "y" -o "$BUILDNEW" = "yes" ] ;then
echo "清除之前的编译,全部重新编译"
make clean;
else
echo "在以前的基础上继续编译，不清除之前的编译"
fi

OUT_TARGET_NAME=$(sed -n 1p $ConfigFile)
PRODUCT_VER1=`echo $OUT_TARGET_NAME | awk -F "_" '{print $2}'`
PRODUCT_VER1=`echo $PRODUCT_VER1 | awk -F "=" '{print $2}'`
PRODUCT_VER2=`echo $OUT_TARGET_NAME | awk -F "_" '{print $3}'`
PRODUCT_VER3=${PRODUCT_VER1}_${PRODUCT_VER2}
export OUT_TARGET=$ANDROID_BUILD_SRC/out/target/product/$PRODUCT_VER3
echo $OUT_TARGET
mkdir -p $OUT_TARGET

if [[ $TARGET_BUILD_VARIANT ]]; then
	rm -rf $VENDOR_OUT_TARGET
	unzip -o $VENDOR_IDH_TARGET/proprietories-${BOARD}.zip -d $VENDOR_TEST_TARGET
	cp -rf $VENDOR_OUT_TARGET $ANDROID_BUILD_SRC
        rm -rf $VENDOR_OUT_TARGET
fi

kheader
IDH_PROP_ZIP=vendor/sprd/release/IDH/proprietories-${BOARD}.zip 

MODEM=$(sed -n 2p $ConfigFile)
export ModemVersion=$MODEM
echo $ModemVersion

./vendor/sprd/release/basePac/replacebin.sh || return 1

make update-api;

make -j24 2>&1 | tee build.log

date

./pac
#./vendor/sprd/build/buildpac/pack.sh

