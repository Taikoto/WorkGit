
@set version=1.7.2Normal
@echo =============================================================
@echo II                     Log gripping tool                II
@echo II                     1.7.2  Normal                    II

@echo =============================================================

@adb devices

@echo   1.  Start creating dir

@set /p dirName=Please enter the folder name and return confirmation, or use the default name for direct return:
@if defined dirName (
    @echo  dirName = [%dirName%]
) else (
    @set dirName=TECON_%version%_pctime_%DATE:~0,4%-%DATE:~5,2%-%DATE:~8,2%
    @echo  dirName = [%dirName%]
)

@mkdir %dirName%
@set pcTime= %DATE% %TIME:~0,8%

@echo  Logtool %version%  >> %dirName%/pcTime
@echo  %pcTime%  >> %dirName%/pcTime

adb shell date > %dirName%/phoneTime
@echo       Creating End
@echo ---------------------------------------------------
@echo   2.  Start capturing screenshots
@echo 。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
@echo I  care：                                                                             I
@echo I      When the phone falls into an abnormal state, the screenshot fails              I
@echo I      The script will stay down.                                                     I
@echo I      If this is the case, press the "CTRL + C" key combination once                 I
@echo I      If the ^c abort batch operation (y/n) appears?                                 I
@echo I      Please press the "CTRL + C" key again                                          I
@echo I      When "3". Crawl process Information "appears                                   I
@echo I      Indicates that the script completes the screenshot step and starts to follow upI
@echo 。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
@adb shell screencap -p /sdcard/PrintScreen.png
@adb pull /sdcard/PrintScreen.png %dirName%/PrintScreen.png
@echo       Capture screenshot End
@echo ---------------------------------------------------
@echo   4.  Start crawling ANR information
@adb pull /sdcard/mtklog %dirName%/mtklog
@echo       Capture ANR information End
@echo ---------------------------------------------------
@echo   5.  Start crawling ANR information
@adb pull /data/anr %dirName%/ANRinfo
@echo       Capture ANR information End
@echo ---------------------------------------------------
@echo   6.  Start crawling ANR information
@adb pull /sdcard/crash %dirName%/crash
@echo       Capture ANR information End
@echo ---------------------------------------------------


call alluserlog.bat


@echo ===================================================
@echo II                      End                      II
@echo ===================================================

pause