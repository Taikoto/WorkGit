#!/bin/bash

echo " "
echo "Copy building result."
echo "Start!"

project=tb8765ap1_bsp_1g
dir=out/target/product/$project
versionname=$(grep ro.denqin.version=* ./out/target/product/$project/vendor/build.prop)
echo ${versionname}

releaseDir=../release/${versionname#*=}
mkdir -p $releaseDir
echo $releaseDir

#project=$1

#ramdisk=${dir}/ramdisk.img
#cp ${ramdisk} ${releaseDir}

vmlinux=${dir}/obj/KERNEL_OBJ/vmlinux
cp ${vmlinux} ${releaseDir}

scatter=${dir}/*Android_scatter.txt
echo ${scatter}

cp ${scatter} ${releaseDir}

cat ${scatter} | while  read line
do 
 # echo ${line}
  lineheader=${line:0:9}
if [ "$lineheader"x = "file_name"x  ]; then
  #echo ${line}
  filename=${line:11}
  if [ "$filename" != "NONE"  ]; then
     echo ${filename}
     cp ${dir}/${filename} ${releaseDir}
  fi  
fi
done

#db
result=$(echo ${project} | grep "37")
if [[ "$result" != "" ]]
then
echo "mt6737"
apdb=${dir}/obj/CGEN/APDB*
cp -f ${apdb} ${releaseDir}
bpdb=out/target/product/$project/vendor/etc/mddb/BPLGUInfo*
###bpdb=vendor/mediatek/proprietary/custom/$project/modem/*/BPLGUInfo*
cp ${bpdb} ${releaseDir}
else
echo "mt6739"
apdb=${dir}/obj/CGEN/APDB*
cp -f ${apdb} ${releaseDir}
bpdb=out/target/product/$project/vendor/etc/mddb/MDDB_Info*
###bpdb=vendor/mediatek/proprietary/custom/$project/modem/*/BPLGUInfo*
cp ${bpdb} ${releaseDir}
fi

