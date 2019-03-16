
@set version=1.7.2Normal
@echo =============================================================
@echo II                     Log gripping tool                II
@echo II                     1.7.2  Normal                    II

@echo =============================================================

@set pcTime= %DATE% %TIME:~0,8%

@echo  Logtool %version%  >> %dirName%/pcTime
@echo  %pcTime%  >> %dirName%/pcTime
@echo ---------------------------------------------------
@echo   2.  Start Crawl kernel log
@adb shell cat /proc/kmsg > %dirName%/kernel.log
@echo       Crawl Kernel log end
@echo ---------------------------------------------------


@echo ===================================================
@echo II                      End                      II
@echo ===================================================

pause