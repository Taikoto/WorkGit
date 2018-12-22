#!/bin/bash

echo "this is fileCompareMerge.sh,其作用是把fileCompareMerge下面的文件的内容逐行替换掉同名目标文件的对应内容"
fileArr=()
file1Arr=()
project_del_dir="$ProjectName/fileCompareMerge"
function fund_f2()
{
#   echo fund_f2:$1,$2,$3;
#   sed -i "s/^$1.*$/$2/g" $filename2
#   sed -i "s/^$1.*$/$2/g" $3
   sed -i "s/^$1 .*$/$2/g" $3
   sed -i "s/^$1=.*$/$2/g" $3

}

function find_f1()
{
   Line=0;
   while read -r line
   do
      ((Line=Line+1));
      #echo "===========第$Line行================"
      #echo "find_f1:$line"
      a=$line
      OLD_IFS="$IFS" 
      IFS="=" 
      arr=($a) 
      IFS="$OLD_IFS" 
#      for s in ${arr[@]} 
#      do 
#	 echo "$s" 
#      done
      if [ ! ${arr[0]} ]; then
         echo "IS NULL"
      else
         #echo "NOT NULL"
         IFS="|"
 #       fund_f2 ${arr[0]} $line;
         tmp=$(echo ${arr[0]} | sed 's/ //g')
#	 echo t: $tmp
         fund_f2 $tmp $line $2;	
      fi 
 #  done < $filename
   done < $1
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
       fi
    done
}
#开始执行的地方
main;




