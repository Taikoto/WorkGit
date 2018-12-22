#!/bin/bash

projectArr=()
haveSelectProject='no'
g_selectresult="true"
export DefaultProject=default
export ProjectName=$1
export projectpath=$(cd "$(dirname "$0")"; pwd)

function printReadMe()
{
   readMe=$1/readMe.txt
   if [ ! -f "$readMe" ]; then
       echo $readMe不存在
       return
   fi
   while read -r line
   do
      echo "项目简介:$line"
   done < $readMe
 #  cat $readMe
}
function getProjectName()
{
   echo $projectpatha
   for dir in $(ls $projectpatha)
   do
       if [ -d $dir ] ; then
          projectArr=(${projectArr[*]} $dir)
	  printReadMe $dir;
	fi
      # [ -d $dir ] && projectArr=(${projectArr[*]} $dir)
   done
   echo 项目个数: num=${#projectArr[@]} 
   echo 项目型号: ${projectArr[@]}
}

function handleSelectproject()
{
   if [ "$haveSelectProject" = "yes" ] ;then
	echo 你选择的项目存在,你最终选择的项目是:$ANS
	g_selectresult='true'
   else
	echo "你选择的项目不存在，请重新选择"
	g_selectresult='false'
    fi	
}
function waitSelectProject()
{
    echo -n "请从上面的项目型号里面选择并输入要编译的项目型号:"
    read ANS
    echo 你输入的项目是:$ANS
    for i in ${projectArr[@]}
    do
       #[ "$i" == "$ANS" ] && echo "yes"
       [ "$i" == "$ANS" ] && haveSelectProject='yes'
    done
}
function selectProject()
{
    while [[ "$g_selectresult" != 'true' ]]  
    do   
         waitSelectProject;
         handleSelectproject;
    done
    ProjectName=$ANS
    echo 系统要编译的项目是:$ProjectName
    rm -rf ProjectName.txt
    echo $ProjectName >> ProjectName.txt
}
function initProject()
{
    getProjectName;
    if [ ${#projectArr[@]} -gt 0 ] ;then
       selectProject;
    else
      echo 没有发现项目
    fi
}
function fileCompareMerge()
{
    sh fileCompareMerge.sh
}
function fileAppendMerge()
{
    sh fileAppendMerge.sh
}
function fileReplace()
{
    sh fileReplace.sh
}
function fileRestoryDefault()
{
    echo "将差异化的参数和文件恢复为默认的"
    sh fileRestoryDefault.sh
}
function main()
{
    rm -rf ProjectName.txt
    echo $ProjectName >> ProjectName.txt
    if [ "$g_selectresult" = "true" ] ;then
       if [ "$ProjectName" = "$DefaultProject" ] ;then
           echo "选择的是默认项目，所有参数和文件都将恢复和使用默认的,不进行差异化处理了"
	   fileRestoryDefault;
       else
           echo 选择的不是默认项目,是$ProjectName，将继续进行差异化处理
	   fileRestoryDefault;
           fileAppendMerge;
           fileCompareMerge;
           fileReplace;
       fi
    else
       echo 没有发现项目,不再继续选择项目
    fi
}

#代码执行的地方
main;