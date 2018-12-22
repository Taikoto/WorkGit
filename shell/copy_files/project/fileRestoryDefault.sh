#!/bin/bash

echo "this is fileRestoryDefault.sh，其作用是把defaultFiles下面的文件替换掉同名目标文件"

fileArr=()
file1Arr=()
project_del_dir="$DefaultProject/defaultFiles"

function find_f1()
{

   echo "copy from $1 to $2" 	
   cp -rf $1 $2
}

function getdir()
{
    for element in `ls $1`
    do  
        dir_or_file=$1"/"$element
        if [ -d $dir_or_file ]
        then 
            getdir $dir_or_file
        else
            #echo $dir_or_file
	    fileArr=(${fileArr[*]} $dir_or_file)
	    tmp=$srcpath/${dir_or_file/$project_del_dir\//}
	    file1Arr=(${file1Arr[*]} $tmp)
        fi  
    done
}

function main()
{
    if [ ! -d "$project_del_dir" ]; then
       echo $project_del_dir不存在，退出
       exit
    fi
    getdir $project_del_dir;
    echo 要合并的内容所在文件: ${fileArr[@]}
    echo 被合并的文件是: ${file1Arr[@]}
    for (( i = 0; i < ${#file1Arr[@]}; ++i )); 
    do 
       if [ -f "${file1Arr[i]}" ]; then
          echo ${file1Arr[i]}存在
	  find_f1 ${fileArr[i]} ${file1Arr[i]};
       else
          echo ${file1Arr[i]}不存在，没法替换
       fi
    done
}
#开始执行的地方
main;




