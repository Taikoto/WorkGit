#!/bin/bash
   #echo $#
   if [ $# != 2 ]; then
      echo -e "\033[31mplease input projectname platform (e.g build_release.sh demo_test mt8765)\033[0m"
      kill 0
   fi
   #项目名 
   projectname=$1
   
   #平台名称 例如mt6737  mt6739
   platform=$2
   if [[ "$platform" != "mt8765" ]];  then 
      echo -e "\033[31mplease  input projectname platform (e.g build_release.sh demo_test mt8765)\033[0m"
      kill 0
   fi
   
   #获取项目所在路径
   bigproject=${projectname%%_*}
   path=${platform}/${bigproject}/${projectname}
   src=./eiot/project/${path}
   if [ ! -d  "$src" ]; then
     echo -e "\033[31m$src not exist\033[0m"
     kill 0
   fi
   
   #保存项目信息 
   #mtkbase mtk参考项目名称
   #projectname 当前编译的项目名称
   pjmk=$(find $src -name 'ProjectConfig.mk')
   base=${pjmk%/ProjectConfig.mk}
   base=${base##*/}
   echo $base > mtkbase
   echo "$projectname"> projectname
   
   #拷贝modem
   ./eiot/tools/modem.sh $pjmk $platform
   
   #拷贝驱动 eiot/driver
   driver=eiot/driver
   cp -ar ${driver}/* ./   
   
   #拷贝项目配置文件  eiot/project/xxxxxxx
   cp -ar ${src}/* ./
  