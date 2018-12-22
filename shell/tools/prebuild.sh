#!/bin/bash

echo "prebuild.sh"
oldPath=$PWD
echo $oldPath

project=$(cat mtkbase)
echo $project
projectPath=device/mediateksample/${project}/ProjectConfig.mk
echo $projectPath
outPath='out/target/product/ProjectConfig.h'
outdir='out/target/product'
if [ ! -d $outdir ]; then
       mkdir -p $outdir
fi 
java -jar eiot/tools/mk2h.jar  $projectPath  ${outPath}


prebuildPath=eiot/prebuild/
find $prebuildPath  -type f | while read line;
do
  echo "================"$line"=================="
      extension=${line##*.}
      projectH=$PWD/$outPath
      oldfilepath=$PWD/$line
      newfilepath=$PWD/${line#*prebuild/}  
      echo "extension:"$extension
  if [[ "$extension" == "java" ]] || [[ "$extension" == "xml" ]] || [[ "$extension" == "aidl" ]] ||  [[ "$extension" == "te" ]]; then 
      gcc -E -C -P -include $projectH  -x c $oldfilepath  -o $newfilepath	  
  else
   #其它文件通过拷贝的方式
   np=${newfilepath%/*}
   if [ ! -d $np ]; then
       mkdir -p $np
   fi   
   cp  -ar $oldfilepath $np
  fi
done
