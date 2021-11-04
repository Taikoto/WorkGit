#!/bin/bash

# 打印函数
function print_red(){
    echo -e "\033[31m $* \033[0m"
}

function print_green(){
    echo -e "\033[32m $* \033[0m"
}

function print_yellow(){
    echo -e "\033[33m $* \033[0m"
}

function print_white(){
    echo -e "\033[37m $* \033[0m"
}

#1. Handle input parameters
CMD_NUM="$#"
if [ $CMD_NUM -lt 2 ];then
    print_red "Error: Parameters is not correct!"
    print_red "Usage: ./exeDelay.sh [Delay time(minutes)] [Action1] [Action2] [Action3] ..."
    exit 1
else
    DELAY_MIN=$1 # delay time
    index=0
    for arg in "$@"                     
    do
    if [ $index -eq 0 ];then
        print_yellow "Delay $DELAY_MIN minutes will execute commands :"
    else
        EXE_CMD[$index]=$arg # commands
        print_white "($index) -> ${EXE_CMD[$index]}"
    fi
     let index+=1
    done
    print_yellow "Wait..."
fi

#2. Start timer
while [ $DELAY_MIN -gt 0 ] 
do 
sleep 60
let DELAY_MIN-=1
print_yellow "Commands will execute after $DELAY_MIN minutes..."
done

#3. Execute commands
for ((i=1;i<=$CMD_NUM;i=i+1))
do
${EXE_CMD[$i]}
done

print_green 'Delay execute finish!!!'
