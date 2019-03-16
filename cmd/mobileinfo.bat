echo

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
@echo   3.  Start Crawl kernel process list
@adb shell ps > %dirName%/Kernel_process_list
@echo       Crawl Kernel process list end
@echo ---------------------------------------------------
@echo   4.  Start crawling CPU information
@adb shell dumpsys cpuinfo > %dirName%/dump_cpuinfo
@echo       Crawl CPU Information End
@echo ---------------------------------------------------
@echo   5.  Start crawling memory information
@adb shell dumpsys meminfo > %dirName%/dump_meminfo
@echo       Crawl Memory Information End
@echo ---------------------------------------------------
@echo   6.  Start capturing power information
@adb shell dumpsys battery > %dirName%/dump_batteryinfo
@echo       Capture Power Information End
@echo ---------------------------------------------------
@echo   7.  Start crawling mobilelog & aee_exp & anr
@adb pull /storage/sdcard0/mtklog/mobilelog %dirName%/mtklog/mobilelog
@adb pull /storage/sdcard1/mtklog/mobilelog %dirName%/mtklog/mobilelog

@adb pull /storage/sdcard0/mtklog/aee_exp %dirName%/mtklog/aee_exp
@adb pull /storage/sdcard1/mtklog/aee_exp %dirName%/mtklog/aee_exp

@adb pull /storage/sdcard0/mtklog/anr %dirName%/mtklog/anr
@adb pull /storage/sdcard1/mtklog/anr %dirName%/mtklog/anr
@echo       Capture mobilelog & aee_exp& anr End
@echo ---------------------------------------------------

@echo   8.  Start crawling ANR information
@adb pull /data/anr %dirName%/ANRinfo
@echo       Capture ANR information End
@echo ---------------------------------------------------
@echo   9.  Start crawling db information
@adb pull /data/aee_exp %dirName%/data_aee_exp
@echo       Capture db information End
@echo ---------------------------------------------------
@echo   10.  Start fetching memory partition information
@adb shell df > %dirName%/memory_partition_info
@echo       Crawl Memory partition information end
@echo ---------------------------------------------------
@echo   11. Start fetching special information
@adb pull /data/mobilelog %dirName%/data_mobilelog
@adb pull /data/core %dirName%/data_core
@adb pull /data/tombstones %dirName%/tombstones
@echo       Grab Special Info End
@echo ---------------------------------------------------
@echo   12. Start crawling all installation package information
@cd %dirName%
@mkdir packageInfo
@cd ..
@adb shell pm list package > %dirName%/packageInfo/packageList
@adb shell pm list package -f > %dirName%/packageInfo/packageDirList
@adb shell pm list package -d > %dirName%/packageInfo/disabledPackageList
@adb shell pm list package -s > %dirName%/packageInfo/systemPackageList
@adb shell pm list package -3 > %dirName%/packageInfo/thirdPackageList
@adb shell pm get-install-location >> %dirName%/packageInfo/otherInfo
@adb shell pm list users >> %dirName%/packageInfo/otherInfo
@echo       Crawl all installation package information end

@echo ===================================================
@echo II                      End                      II
@echo ===================================================

pause