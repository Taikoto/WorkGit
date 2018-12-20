@echo off & setlocal enabledelayedexpansion
set count=0
set current=0
:step
::改num值设置一个循环的时间，单位是S
set/a num=120
set/a count+=1

for /l %%a in (!num! -1 1) DO (
 echo 剩余时间: [%%a]
 set/a current=count
 if not %current% == 0 echo.第 [%current%] 次
 timeout /t 1 /nobreak>nul & cls
)
@adb reboot || echo 故障前已循环%count%次 && pause
echo.已循环%count%次
::改"%count%"=="2000" 设置最大循环次数
if "%count%"=="2000" pause&&echo.循环2000次啦！

goto step